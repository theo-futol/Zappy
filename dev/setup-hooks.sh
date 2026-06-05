#!/usr/bin/env bash

# Safe working directory: script is in dev/, move to repo root
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR" || exit 1

echo
echo "=== Husky + Commitlint + clang-format setup ==="

echo "1) Installing all required dependencies (npm)..."
if command -v npm >/dev/null 2>&1; then
	npm install --save-dev husky @commitlint/cli @commitlint/config-conventional clang-format
	echo "-> npm devDependencies installed."
else
	echo "ERROR: npm not found. Install Node.js/npm and re-run this script." >&2
	exit 1
fi

echo "----"

echo "2) Initializing husky and creating hooks..."
# Use modern husky install flow
npx husky init

# Add commit-msg hook to run commitlint
echo "npx --no-install commitlint --edit "$1"" > .husky/commit-msg
echo "-> .husky/commit-msg created."

# Add pre-commit hook to format C++ files
echo "
files=$(git diff --cached --name-only -- '*.cpp')

if [ -n "$files" ]; then
  echo "Running clang-format on staged C++ files..."
  for file in $files; do
    clang-format -i "$file"
    git add "$file"
  done
  echo "clang-format completed and changes staged, you can now commit again."
else
  echo "No staged C++ files to format."
fi
" > .husky/pre-commit
echo "-> .husky/pre-commit created."

echo "----"

echo "3) Creating commitlint config file..."
cat > commitlint.config.js <<'JS'
module.exports = {
	extends: ['@commitlint/config-conventional'],
};
JS
echo "-> commitlint.config.js created."

echo "----"

echo "4) Finalizing: ensuring hooks are executable"
if [ -d .husky ]; then
	chmod +x .husky/* || true
	echo "-> Husky hooks are executable."
else
	echo "WARNING: .husky directory not found. Husky may not have been installed correctly." >&2
fi

echo
echo "Setup complete — Husky, Commitlint and clang-format are configured."