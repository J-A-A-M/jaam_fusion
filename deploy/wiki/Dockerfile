# Stage 1 - build the static site.
# Material for MkDocs already ships mkdocs + git, so we only add the
# revision-date plugin on top.
FROM squidfunk/mkdocs-material:9.7.6 AS builder

RUN pip install --no-cache-dir --root-user-action=ignore \
        mkdocs-git-revision-date-localized-plugin

WORKDIR /docs

# Copy the whole repo so the git-revision-date-localized plugin sees the full
# history (.git). COPY makes the tree root-owned, matching the build user, so
# no git safe.directory tweak is needed (and this image pre-trusts /docs anyway).
COPY . .

RUN mkdocs build --clean --strict

# Stage 2 - serve the built site from a clean nginx image. Nothing from the
# builder stage (mkdocs, git, .git, sources) ends up here - only static files.
FROM nginx:alpine
COPY --from=builder /docs/site /usr/share/nginx/html
