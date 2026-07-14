#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
MODEL_ID="onnx-community/gemma-3-1b-it-ONNX"
MODEL_REVISION="a58439f40017d3b99c7d378ff525e54e0ba08ebf"
MODEL_DIR="$ROOT/frontend/models/$MODEL_ID"
RUNTIME_DIR="$ROOT/frontend/vendor/transformers"
TRANSFORMERS_VERSION="4.2.0"

if ! command -v hf >/dev/null 2>&1; then
  printf '%s\n' "La commande hf est nécessaire : https://huggingface.co/docs/huggingface_hub/guides/cli" >&2
  exit 1
fi
if ! command -v npm >/dev/null 2>&1; then
  printf '%s\n' "npm est nécessaire pour installer le runtime JavaScript local." >&2
  exit 1
fi

TEMPORARY=$(mktemp -d "${TMPDIR:-/tmp}/linux-doctor-ai.XXXXXX")
trap 'rm -rf -- "$TEMPORARY"' EXIT HUP INT TERM

mkdir -p "$MODEL_DIR" "$RUNTIME_DIR"

printf 'Installation de Transformers.js %s…\n' "$TRANSFORMERS_VERSION"
npm install --prefix "$TEMPORARY/runtime" --silent --no-audit --no-fund --ignore-scripts \
  "@huggingface/transformers@$TRANSFORMERS_VERSION"
sed \
  -e 's|from"onnxruntime-common"|from"./ort.webgpu.bundle.min.mjs"|g' \
  -e 's|from"onnxruntime-web/webgpu"|from"./ort.webgpu.bundle.min.mjs"|g' \
  "$TEMPORARY/runtime/node_modules/@huggingface/transformers/dist/transformers.web.min.js" \
  >"$TEMPORARY/transformers.web.min.js"
install -m 0644 "$TEMPORARY/transformers.web.min.js" "$RUNTIME_DIR/transformers.web.min.js"
for runtime_file in \
  ort-wasm-simd-threaded.mjs ort-wasm-simd-threaded.wasm \
  ort-wasm-simd-threaded.asyncify.mjs ort-wasm-simd-threaded.asyncify.wasm \
  ort-wasm-simd-threaded.jsep.mjs ort-wasm-simd-threaded.jsep.wasm \
  ort-wasm-simd-threaded.jspi.mjs ort-wasm-simd-threaded.jspi.wasm
do
  install -m 0644 "$TEMPORARY/runtime/node_modules/onnxruntime-web/dist/$runtime_file" \
    "$RUNTIME_DIR/$runtime_file"
done
install -m 0644 "$TEMPORARY/runtime/node_modules/onnxruntime-web/dist/ort.webgpu.bundle.min.mjs" \
  "$RUNTIME_DIR/ort.webgpu.bundle.min.mjs"
install -m 0644 "$TEMPORARY/runtime/node_modules/@huggingface/transformers/LICENSE" \
  "$RUNTIME_DIR/LICENSE"

printf 'Téléchargement de %s en int8 (environ 1,05 Go)…\n' "$MODEL_ID"
hf download "$MODEL_ID" \
  --revision "$MODEL_REVISION" \
  --local-dir "$MODEL_DIR" \
  config.json generation_config.json special_tokens_map.json tokenizer.json \
  tokenizer_config.json onnx/model_int8.onnx

printf '{"schema":"linux-doctor.local-ai","version":1,"model_id":"%s","revision":"%s","dtype":"int8","transformers_js":"%s"}\n' \
  "$MODEL_ID" "$MODEL_REVISION" "$TRANSFORMERS_VERSION" \
  >"$TEMPORARY/local-ai-manifest.json"
install -m 0644 "$TEMPORARY/local-ai-manifest.json" \
  "$ROOT/frontend/models/local-ai-manifest.json"

printf '%s\n' "Assistant local installé. Relancez le serveur puis ouvrez Future Lab."
