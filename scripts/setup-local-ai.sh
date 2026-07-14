#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
MODEL_ID="onnx-community/gemma-3-270m-it-ONNX"
MODEL_REVISION="2dbbfdb1b59bd034eb959428c6a7da9dd7ea27f0"
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

printf 'Téléchargement de %s en fp16 (environ 570 Mo)…\n' "$MODEL_ID"
hf download "$MODEL_ID" \
  --revision "$MODEL_REVISION" \
  --local-dir "$MODEL_DIR" \
  config.json generation_config.json special_tokens_map.json tokenizer.json \
  tokenizer_config.json onnx/model_fp16.onnx onnx/model_fp16.onnx_data

# The upstream tokenizer omits legacy flags that Transformers.js 4.2 reads
# while encoding a plain prompt. Normalize them in the installed local copy.
sed \
  -e '/  "add_bos_token":/d' \
  -e '/  "add_eos_token":/d' \
  -e 's/  "backend":/  "add_bos_token": true,\
  "add_eos_token": false,\
  "backend":/' \
  "$MODEL_DIR/tokenizer_config.json" >"$TEMPORARY/tokenizer_config.json"
install -m 0644 "$TEMPORARY/tokenizer_config.json" "$MODEL_DIR/tokenizer_config.json"

# Remove weights used by earlier development builds so a successful setup
# leaves only the supported fp16 model on disk.
rm -rf -- "$ROOT/frontend/models/onnx-community/gemma-3-1b-it-ONNX"
rm -f -- \
  "$MODEL_DIR/onnx/model_quantized.onnx" "$MODEL_DIR/onnx/model_quantized.onnx_data" \
  "$MODEL_DIR/onnx/model_q4f16.onnx" "$MODEL_DIR/onnx/model_q4f16.onnx_data"

printf '{"schema":"linux-doctor.local-ai","version":1,"model_id":"%s","revision":"%s","dtype":"fp16","transformers_js":"%s"}\n' \
  "$MODEL_ID" "$MODEL_REVISION" "$TRANSFORMERS_VERSION" \
  >"$TEMPORARY/local-ai-manifest.json"
install -m 0644 "$TEMPORARY/local-ai-manifest.json" \
  "$ROOT/frontend/models/local-ai-manifest.json"

printf '%s\n' "Assistant local installé. Relancez le serveur puis ouvrez Future Lab."
