#!/usr/bin/env bash
set -euo pipefail

# Unified release script: prepares Docker, logs into GHCR, builds & pushes devcontainer image.

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Load project .env if present
if [[ -f "${SCRIPT_DIR}/../.env" ]]; then
  # shellcheck disable=SC1091
  source "${SCRIPT_DIR}/../.env"
elif [[ -f ".env" ]]; then
  # fallback if run from repo root
  # shellcheck disable=SC1091
  source ./.env
fi

# Defaults
REGISTRY=${REGISTRY:-ghcr.io}
OWNER=${OWNER:-0xfischer}
REPO=${REPO:-parking_garage_control_system}
IMAGE=${IMAGE:-${REGISTRY}/${OWNER}/${REPO}-dev:latest}
TAG=${TAG:-}
ESP_IDF_VERSION=${ESP_IDF_VERSION:-v5.2.2}
PLATFORM=${PLATFORM:-linux/amd64}
DOCKERFILE=${DOCKERFILE:-.devcontainer/Dockerfile}
DOCKER_CONTEXT=${DOCKER_CONTEXT:-.}
USE_BUILDX=${USE_BUILDX:-1}

usage() {
  cat <<EOF
Usage: tools/release_devcontainer_to_github.sh [--tag <TAG>] [--no-buildx] [--image <IMAGE>] [--owner <OWNER>] [--repo <REPO>]

Environment variables:
  REGISTRY, OWNER, REPO, IMAGE, TAG, ESP_IDF_VERSION, PLATFORM, DOCKERFILE, DOCKER_CONTEXT, USE_BUILDX, GITHUB_TOKEN
Reads variables from .env if present.

Examples:
  TAG=v5.2.2 tools/release_devcontainer_to_github.sh
  tools/release_devcontainer_to_github.sh --tag v5.2.2 --image ghcr.io/0xfischer/parking_garage_control_system-dev:latest
EOF
}

# Parse simple args
while [[ $# -gt 0 ]]; do
  case "$1" in
    --tag)
      TAG="$2"; shift 2 ;;
    --no-buildx)
      USE_BUILDX=0; shift ;;
    --image)
      IMAGE="$2"; shift 2 ;;
    --owner)
      OWNER="$2"; shift 2 ;;
    --repo)
      REPO="$2"; shift 2 ;;
    -h|--help)
      usage; exit 0 ;;
    *)
      echo "Unknown argument: $1" >&2; usage; exit 2 ;;
  esac
done

# Ensure Docker CLI is available
if ! command -v docker >/dev/null 2>&1; then
  echo "Docker nicht gefunden. Bitte Docker installieren und erneut versuchen." >&2
  exit 127
fi

# Ensure Docker daemon and permissions are OK
if ! docker info >/dev/null 2>&1; then
  echo "Docker ist installiert, aber es fehlen Berechtigungen oder der Daemon läuft nicht." >&2
  echo "Tipps:" >&2
  echo "  - Prüfe, ob der Docker-Daemon läuft: 'systemctl status docker'" >&2
  echo "  - Füge deinen Benutzer der 'docker'-Gruppe hinzu und melde dich neu an:" >&2
  echo "      sudo usermod -aG docker $USER" >&2
  echo "  - Alternativ den Befehl mit 'sudo' ausführen." >&2
  exit 1
fi

# Ensure a valid Docker context is selected
# Some environments misconfigure the current context to a non-existent one (error mentions context ".")
# Prefer an explicit context if provided via DOCKER_CONTEXT_NAME, else force 'default' when current is broken.
CURRENT_CTX="$(docker context show 2>/dev/null || echo default)"
TARGET_CTX="${DOCKER_CONTEXT_NAME:-$CURRENT_CTX}"

# If the target context cannot be inspected, fall back to 'default'
if ! docker context inspect "$TARGET_CTX" >/dev/null 2>&1; then
  TARGET_CTX=default
fi

# Switch context only if different
if [ "$TARGET_CTX" != "$CURRENT_CTX" ]; then
  echo "Switching Docker context to '$TARGET_CTX'"
  docker context use "$TARGET_CTX" >/dev/null 2>&1 || echo "Warning: failed to switch Docker context; continuing with current context '$CURRENT_CTX'"
fi

# Ensure buildx exists and a builder is active
if ! docker buildx version >/dev/null 2>&1; then
  echo "Docker Buildx ist nicht verfügbar. Bitte Docker-Plugin installieren/aktivieren." >&2
  exit 1
fi

# Ensure a usable buildx builder exists; create a named one if none is active
if ! docker buildx ls | grep -q "\*"; then
  echo "Creating and using buildx builder 'repo-builder'"
  docker buildx create --use --name repo-builder >/dev/null 2>&1 || true
fi

# Login to GHCR
TOK=${GITHUB_TOKEN:-}
if [[ -z "$TOK" ]]; then
  echo "GITHUB_TOKEN not set. Provide with 'GITHUB_TOKEN=...' or add to .env" >&2
  exit 1
fi

echo "Logging in to $REGISTRY as '$OWNER'"
echo "$TOK" | docker login "$REGISTRY" -u "$OWNER" --password-stdin

echo "Building container image: ${IMAGE}"
if [[ "${USE_BUILDX}" == "1" ]]; then
  echo "Building (buildx) image for platform ${PLATFORM}"
  docker buildx build -f "${DOCKERFILE}" -t "${IMAGE}" \
    --build-arg "ESP_IDF_VERSION=${ESP_IDF_VERSION}" \
    --platform "${PLATFORM}" "${DOCKER_CONTEXT}" --push
else
  docker build -f "${DOCKERFILE}" -t "${IMAGE}" \
    --build-arg "ESP_IDF_VERSION=${ESP_IDF_VERSION}" \
    "${DOCKER_CONTEXT}"
  docker push "${IMAGE}"
fi

if [[ -n "${TAG}" ]]; then
  echo "Tagging ${IMAGE} as ${REGISTRY}/${OWNER}/${REPO}-dev:${TAG}"
  docker tag "${IMAGE}" "${REGISTRY}/${OWNER}/${REPO}-dev:${TAG}"
  echo "Pushing tagged image: ${REGISTRY}/${OWNER}/${REPO}-dev:${TAG}"
  docker push "${REGISTRY}/${OWNER}/${REPO}-dev:${TAG}"
else
  echo "No TAG provided; skipping additional tag push."
fi

echo "Devcontainer release to GitHub Container Registry completed."
