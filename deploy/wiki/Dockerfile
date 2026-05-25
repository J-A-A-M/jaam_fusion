# syntax=docker/dockerfile:1
FROM python:3.14-slim AS builder

# git is required by the mkdocs git-revision-date-localized plugin
RUN apt-get update \
    && apt-get install -y --no-install-recommends git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

# Install dependencies first so this layer stays cached unless requirements change
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Only the MkDocs build inputs. docs_dir is "wiki" and holds all assets/css/js.
COPY mkdocs.yml ./
COPY wiki/ ./wiki/

# Build the static site. .git is bind-mounted read-only for this step only, so
# the large (and possibly credential-bearing) history is available to the
# git-revision-date-localized plugin without ever landing in an image layer.
# MKDOCS_BUILD_STRICT is only compared, never interpolated into the command, so
# a build arg from untrusted CI input cannot inject shell.
ARG MKDOCS_BUILD_STRICT=false
RUN --mount=type=bind,source=.git,target=.git \
    if [ "$MKDOCS_BUILD_STRICT" = "true" ]; then \
        mkdocs build --clean --strict --verbose; \
    else \
        mkdocs build --clean; \
    fi

FROM nginx:alpine
COPY --from=builder /build/site /usr/share/nginx/html
