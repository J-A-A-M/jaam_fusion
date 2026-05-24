FROM python:3.13-slim AS builder

# git is required by the mkdocs git-revision-date-localized plugin
RUN apt-get update \
    && apt-get install -y --no-install-recommends git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY . .
ARG MKDOCS_BUILD_FLAGS="--clean"
RUN mkdocs build ${MKDOCS_BUILD_FLAGS}

FROM nginx:alpine
COPY --from=builder /build/site /usr/share/nginx/html
