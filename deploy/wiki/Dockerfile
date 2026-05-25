FROM python:3.13-slim AS builder

# git is required by the mkdocs git-revision-date-localized plugin
RUN apt-get update \
    && apt-get install -y --no-install-recommends git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

# Install dependencies first so this layer stays cached unless requirements change
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copy only the MkDocs build inputs to keep the build context small
# . docs_dir is "wiki" and holds all assets/css/js; .git is needed
# by the git-revision-date-localized plugin to compute per-page dates.
COPY mkdocs.yml ./
COPY wiki/ ./wiki/
COPY .git/ ./.git/

# Toggle strict validation without interpolating untrusted strings into the
# shell: the arg is only compared, the mkdocs commands themselves are fixed.
ARG MKDOCS_BUILD_STRICT=false
RUN if [ "$MKDOCS_BUILD_STRICT" = "true" ]; then \
        mkdocs build --clean --strict --verbose; \
    else \
        mkdocs build --clean; \
    fi

FROM nginx:alpine
COPY --from=builder /build/site /usr/share/nginx/html
