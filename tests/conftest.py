def pytest_addoption(parser):
    parser.addoption("--ip", required=True, help="Map device IP address")
    parser.addoption("--login", default="admin", help="Web login (default: admin)")
    parser.addoption("--password", default="admin", help="Web password (default: admin)")
