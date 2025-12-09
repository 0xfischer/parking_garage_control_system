# Lokales Testen von GitHub Actions mit `act`

Diese Anleitung erklärt, wie du `act` unter Linux (zsh) installierst und wie du damit GitHub-Workflows lokal ausführst und debuggst.

## Voraussetzungen
- `docker` installiert und lauffähig (Rootless oder mit passenden Gruppenrechten)
- `git` installiert
- zsh als Shell (Standard im Projekt)
- Zugriff auf dieses Repository mit bestehenden `.github/workflows/*`

Optional, aber empfehlenswert:
- GitHub Personal Access Token (PAT) für höhere API‑Limits bei `actions/checkout` usw.

## Installation

Du kannst `act` entweder als Binary installieren oder per Paketmanager.

### Variante A: Binary Installation
```zsh
# Neueste Version ermitteln und installieren
ACT_VERSION=$(curl -s https://api.github.com/repos/nektos/act/releases/latest | jq -r '.tag_name')
curl -L -o /tmp/act.tgz "https://github.com/nektos/act/releases/download/${ACT_VERSION}/act_Linux_x86_64.tar.gz"
sudo tar -C /usr/local/bin -xzf /tmp/act.tgz act
command -v act && act --version
```

### Variante B: Paketmanager (falls verfügbar)
```zsh
# Beispiel mit Homebrew auf Linux
brew install act
```

Falls `jq` fehlt, installiere es mit deinem Paketmanager (z. B. `sudo apt-get install jq`).

## Docker einrichten
- Prüfe, ob Docker läuft:
```zsh
docker info | grep -E 'Server Version|Kernel Version'
```
- Stelle sicher, dass dein Nutzer in der `docker`-Gruppe ist (oder nutze Rootless Docker):
```zsh
sudo usermod -aG docker "$USER"
newgrp docker
```

## Authentifizierung und Limits
`act` verwendet standardmäßig anonyme GitHub API‑Zugriffe, was drosseln kann. Lege optional ein Token an:
```zsh
export GITHUB_TOKEN="<dein_personal_access_token>"
```
Du kannst alternativ eine `.env`-Datei im Projekt anlegen und darin `GITHUB_TOKEN` setzen. `act` lädt automatisch Umgebungsvariablen aus `.env`.

## Runner-Image wählen
Workflows deklarieren meist `runs-on: ubuntu-latest`. `act` mappt das auf Container-Images. Häufige Images:
- `catthehacker/ubuntu:act-latest` (leichtgewichtig)
- `ghcr.io/catthehacker/ubuntu:act-22.04` (Ubuntu 22.04)

Setze ein Standard-Image mit Flag `-P` (Platform Mapping) oder lege `.actrc` an:
```zsh
# Schnellstart mit Ubuntu 22.04
act -P ubuntu-latest=ghcr.io/catthehacker/ubuntu:act-22.04
```

Optional `.actrc` im Repo-Wurzelverzeichnis:
```
-P ubuntu-latest=ghcr.io/catthehacker/ubuntu:act-22.04
```

## Workflows ausführen

Grundbefehl (führt Default-Event `push` aus):
```zsh
act
```

Bestimmtes Event triggern (z. B. `pull_request`):
```zsh
act pull_request
```

Nur einen bestimmten Job ausführen:
```zsh
act -j <job_name>
```

Nur geänderte Dateien berücksichtigen (schneller):
```zsh
act --changed
```

Dry-Run ohne Ausführung (Plan anzeigen):
```zsh
act --dryrun
```

Mehr Logs/Debug:
```zsh
act -v   # verbose
act -W .github/workflows/<file>.yml -v
```

## Secrets und Envs
Secrets kannst du lokal über `.secrets` bereitstellen:
```
MY_SECRET=wert
ANOTHER_SECRET=abc
```
Aufruf mit Secrets:
```zsh
act --secret-file .secrets
```

Umgebungsvariablen aus `.env` werden automatisch geladen. Alternativ:
```zsh
act -s KEY=VALUE -s OTHER=123
```

## Volumes, Caching und Performance
- Standard-Cache-Verzeichnis von `act`: `~/.cache/act`
- Du kannst zusätzliche Volumes mounten, z. B. für Node/NPM oder ESP‑IDF Artefakte:
```zsh
act -b \
  -v "$HOME/.cache/act:/var/cache/act" \
  -v "$HOME/.cache/esp:/root/.cache/esp"
```

`-b` aktiviert Buildkit für schnelleres Docker‑Build (falls im Workflow verwendet).

## Typische Probleme und Lösungen
- Fehlende Runner‑Pakete: Ergänze im Workflow Setup‑Schritte (z. B. `apt-get install`) oder nutze ein passendes Image.
- Netzwerklimits/GitHub Rate Limit: Setze `GITHUB_TOKEN`.
- Docker Permission Denied: Prüfe Gruppenmitgliedschaft `docker` oder Rootless Docker.
- Unterschied zwischen lokalen und GitHub‑Runnern: Achte auf OS‑Versionen, installierte Tools und Pfade.

## Beispiele

Alle Workflows für `push` ausführen, mit explizitem Runner‑Image und Verbose Logs:
```zsh
act push -P ubuntu-latest=ghcr.io/catthehacker/ubuntu:act-22.04 -v
```

Nur den Job `build` im Workflow `.github/workflows/ci.yml` testen:
```zsh
act -W .github/workflows/ci.yml -j build -P ubuntu-latest=ghcr.io/catthehacker/ubuntu:act-22.04
```

Secrets und Token verwenden:
```zsh
export GITHUB_TOKEN="<token>"
act -W .github/workflows/ci.yml --secret-file .secrets -v
```

## ESP‑IDF/Hinweise zum Projekt
Wenn eure Workflows ESP‑IDF nutzen, stelle sicher, dass das Runner‑Image alle Anforderungen erfüllt (Python, CMake/Ninja, `xtensa-esp32-elf` Toolchain, etc.). Für lokale Containerläufe empfiehlt sich ein vorbereiteter Container mit der ESP‑IDF Toolchain oder entsprechende Setup‑Schritte im Workflow.

## Weiterführende Links
- Projekt: https://github.com/nektos/act
- Docker Rootless: https://docs.docker.com/engine/security/rootless/
- Catthehacker Images: https://github.com/catthehacker/docker_images
