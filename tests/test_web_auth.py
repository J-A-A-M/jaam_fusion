#!/usr/bin/env python3
"""
Tests for web authentication in JAAM Fusion.

The tests manage auth state themselves:
- TestAuthDisabled: temporarily disables auth, runs tests, restores state.
- TestAuthEnabled:  temporarily enables auth with test credentials, runs tests, restores state.

Usage:
    pytest tests/test_web_auth.py --ip 192.168.1.100
    pytest tests/test_web_auth.py --ip 192.168.1.100 --login admin --password secret

The IP is pinged before any tests — all are skipped if the device is unreachable.
"""
import subprocess
import sys
from urllib.parse import urlparse
import pytest
import requests

TIMEOUT = 5


# ---------------------------------------------------------------------------
# Session-scoped fixtures
# ---------------------------------------------------------------------------

@pytest.fixture(scope="session")
def device_ip(pytestconfig):
    return pytestconfig.getoption("--ip")


@pytest.fixture(scope="session")
def credentials(pytestconfig):
    return {
        "login": pytestconfig.getoption("--login"),
        "password": pytestconfig.getoption("--password"),
    }


@pytest.fixture(scope="session")
def base_url(device_ip):
    return f"http://{device_ip}"


@pytest.fixture(scope="session", autouse=True)
def ping_device(device_ip):
    result = subprocess.run(
        ["ping", "-c", "1", "-W", "2", device_ip],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    if result.returncode != 0:
        pytest.skip(f"Device {device_ip} is unreachable (ping failed)")


@pytest.fixture(scope="session")
def initial_auth_state(base_url):
    """Detect initial auth state on the device."""
    resp = requests.get(base_url + "/", allow_redirects=False, timeout=TIMEOUT)
    return resp.status_code == 302 and "/login" in resp.headers.get("Location", "")


@pytest.fixture(scope="session")
def admin_session(base_url, credentials, initial_auth_state):
    """A requests.Session with admin access, regardless of initial auth state."""
    session = requests.Session()
    if initial_auth_state:
        resp = session.post(
            base_url + "/login",
            data={"login": credentials["login"], "password": credentials["password"]},
            allow_redirects=False,
            timeout=TIMEOUT,
        )
        if resp.status_code != 302 or "error" in resp.headers.get("Location", ""):
            pytest.skip("Cannot authenticate with provided credentials — check --login/--password")
    return session


def set_param(session, base_url, name, value):
    resp = session.post(base_url + "/parameter", data={"name": name, "value": str(value)}, allow_redirects=False, timeout=TIMEOUT)
    assert resp.status_code == 200, f"Failed to set {name}={value}: {resp.status_code}"


# ---------------------------------------------------------------------------
# Class-scoped fixtures that manage auth state
# ---------------------------------------------------------------------------

@pytest.fixture(scope="class")
def ensure_auth_disabled(base_url, admin_session, initial_auth_state):
    """Disable auth for the test class. Restore to enabled after if it was enabled."""
    if initial_auth_state:
        set_param(admin_session, base_url, "web_auth_enabled", "0")
    yield
    if initial_auth_state:
        set_param(admin_session, base_url, "web_auth_enabled", "1")


@pytest.fixture(scope="class")
def ensure_auth_enabled(base_url, admin_session, credentials, initial_auth_state):
    """Enable auth with test credentials. Disable after if it was originally off."""
    if not initial_auth_state:
        set_param(admin_session, base_url, "web_login", credentials["login"])
        set_param(admin_session, base_url, "web_password", credentials["password"])
        # Enabling auth auto-creates a session — admin_session captures the cookie
        resp = admin_session.post(
            base_url + "/parameter",
            data={"name": "web_auth_enabled", "value": "1"},
            allow_redirects=False,
            timeout=TIMEOUT,
        )
        assert resp.status_code == 200, "Failed to enable auth"
    yield credentials
    if not initial_auth_state:
        # Re-authenticate: tests may have invalidated admin_session's cookie
        # because the device uses a single session token (any new login overwrites it)
        teardown_session, resp = do_login(base_url, credentials["login"], credentials["password"])
        assert resp.status_code == 302 and "error" not in resp.headers.get("Location", ""), \
            "Re-auth for teardown failed"
        set_param(teardown_session, base_url, "web_auth_enabled", "0")


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def do_login(base_url, login, password):
    session = requests.Session()
    resp = session.post(
        base_url + "/login",
        data={"login": login, "password": password},
        allow_redirects=False,
        timeout=TIMEOUT,
    )
    return session, resp


def do_recovery_login(base_url, token):
    session = requests.Session()
    resp = session.post(
        base_url + "/login",
        data={"recovery": token},
        allow_redirects=False,
        timeout=TIMEOUT,
    )
    return session, resp


# ---------------------------------------------------------------------------
# Auth disabled tests
# ---------------------------------------------------------------------------

@pytest.mark.usefixtures("ensure_auth_disabled")
class TestAuthDisabled:
    def test_root_accessible(self, base_url):
        resp = requests.get(base_url + "/", timeout=TIMEOUT)
        assert resp.status_code == 200

    def test_system_info_accessible(self, base_url):
        resp = requests.get(base_url + "/system-info", timeout=TIMEOUT)
        assert resp.status_code == 200

    def test_alerts_info_accessible(self, base_url):
        resp = requests.get(base_url + "/alerts-info", timeout=TIMEOUT)
        assert resp.status_code == 200

    def test_login_page_accessible(self, base_url):
        resp = requests.get(base_url + "/login", timeout=TIMEOUT)
        assert resp.status_code == 200

    def test_static_assets_accessible(self, base_url):
        for path in ["/styles.css", "/scripts.js"]:
            resp = requests.get(base_url + path, timeout=TIMEOUT)
            assert resp.status_code == 200, f"{path} should be accessible"


# ---------------------------------------------------------------------------
# Auth enabled tests
# ---------------------------------------------------------------------------

@pytest.mark.usefixtures("ensure_auth_enabled")
class TestAuthEnabled:
    def test_root_redirects_to_login(self, base_url):
        resp = requests.get(base_url + "/", allow_redirects=False, timeout=TIMEOUT)
        assert resp.status_code == 302
        assert urlparse(resp.headers.get("Location", "")).path == "/login"

    def test_protected_endpoints_redirect_without_session(self, base_url):
        for path in ["/system-info", "/alerts-info", "/map-data", "/ui-schema/models"]:
            resp = requests.get(base_url + path, allow_redirects=False, timeout=TIMEOUT)
            assert resp.status_code == 302, f"{path} should redirect without session"
            assert urlparse(resp.headers.get("Location", "")).path == "/login"

    def test_login_success(self, base_url, ensure_auth_enabled):
        creds = ensure_auth_enabled
        _, resp = do_login(base_url, creds["login"], creds["password"])
        assert resp.status_code == 302
        assert "error" not in resp.headers.get("Location", ""), f"Login failed: {resp.headers.get('Location')}"

    def test_login_success_grants_access(self, base_url, ensure_auth_enabled):
        creds = ensure_auth_enabled
        session, resp = do_login(base_url, creds["login"], creds["password"])
        assert resp.status_code == 302 and "error" not in resp.headers.get("Location", "")
        assert session.get(base_url + "/", timeout=TIMEOUT).status_code == 200

    def test_login_wrong_password(self, base_url, ensure_auth_enabled):
        creds = ensure_auth_enabled
        _, resp = do_login(base_url, creds["login"], "wrong_password_xyz_999")
        assert resp.status_code == 302
        assert "error=1" in resp.headers.get("Location", "")

    def test_login_wrong_login(self, base_url, ensure_auth_enabled):
        creds = ensure_auth_enabled
        _, resp = do_login(base_url, "wrong_user_xyz_999", creds["password"])
        assert resp.status_code == 302
        assert "error=1" in resp.headers.get("Location", "")

    def test_login_empty_submitted_password(self, base_url, ensure_auth_enabled):
        """Submitting empty password when device has a password set → error=1 (wrong credentials)."""
        creds = ensure_auth_enabled
        _, resp = do_login(base_url, creds["login"], "")
        assert resp.status_code == 302
        assert "error=1" in resp.headers.get("Location", "")


    def test_login_invalid_recovery_token(self, base_url):
        _, resp = do_recovery_login(base_url, "invalid_token_xyz_000")
        assert resp.status_code == 302
        assert "error=2" in resp.headers.get("Location", "")

    def test_login_empty_recovery_token_falls_through_to_login(self, base_url):
        # Server ignores recovery="" and falls through to login/password check → error=1
        _, resp = do_recovery_login(base_url, "")
        assert resp.status_code == 302
        assert "error=1" in resp.headers.get("Location", "")

    def test_logout_clears_session(self, base_url, ensure_auth_enabled):
        creds = ensure_auth_enabled
        session, resp = do_login(base_url, creds["login"], creds["password"])
        assert resp.status_code == 302 and "error" not in resp.headers.get("Location", "")
        assert session.get(base_url + "/", timeout=TIMEOUT).status_code == 200

        logout = session.post(base_url + "/logout", allow_redirects=False, timeout=TIMEOUT)
        assert logout.status_code == 302
        assert urlparse(logout.headers.get("Location", "")).path == "/login"

        after = session.get(base_url + "/", allow_redirects=False, timeout=TIMEOUT)
        assert after.status_code == 302
        assert urlparse(after.headers.get("Location", "")).path == "/login"

    def test_login_page_public(self, base_url):
        resp = requests.get(base_url + "/login", timeout=TIMEOUT)
        assert resp.status_code == 200

    def test_static_assets_public(self, base_url):
        for path in ["/styles.css", "/scripts.js"]:
            resp = requests.get(base_url + path, timeout=TIMEOUT)
            assert resp.status_code == 200, f"{path} should be public"


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 tests/test_web_auth.py <ip> [--login <login>] [--password <password>]")
        sys.exit(1)
    sys.exit(pytest.main([__file__, f"--ip={sys.argv[1]}", "-v", *sys.argv[2:]]))
