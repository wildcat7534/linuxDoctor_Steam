const MODEL_ID = 'onnx-community/gemma-3-1b-it-ONNX';
const MODEL_DTYPE = 'int8';
const RUNTIME_MODULE = 'vendor/transformers/transformers.web.min.js';
const WASM_DIRECTORY = 'vendor/transformers/';

let generator = null;
let transformers = null;
let activeDevice = null;

function extractGeneratedText(result) {
  const generated = result?.[0]?.generated_text;
  if (Array.isArray(generated)) return generated.at(-1)?.content || '';
  return typeof generated === 'string' ? generated : '';
}

async function createPipeline(device) {
  return transformers.pipeline('text-generation', MODEL_ID, {
    dtype: MODEL_DTYPE,
    device,
    progress_callback: progress => self.postMessage({ type: 'progress', progress })
  });
}

async function ensureGenerator() {
  if (generator) return generator;
  transformers = await import(new URL(RUNTIME_MODULE, import.meta.url));
  const { env } = transformers;
  env.allowRemoteModels = false;
  env.allowLocalModels = true;
  env.localModelPath = new URL('models/', import.meta.url).href;
  env.useBrowserCache = true;
  env.backends.onnx.wasm.wasmPaths = new URL(WASM_DIRECTORY, import.meta.url).href;
  env.backends.onnx.wasm.numThreads = self.crossOriginIsolated
    ? Math.max(1, Math.min(2, navigator.hardwareConcurrency || 1))
    : 1;

  let preferredDevice = 'wasm';
  if (navigator.gpu) {
    try {
      const adapter = await navigator.gpu.requestAdapter();
      const description = String(adapter?.info?.description || '').toLowerCase();
      const isSoftwareAdapter = adapter?.info?.isFallbackAdapter ||
        description.includes('swiftshader') || description.includes('software');
      if (adapter && !isSoftwareAdapter) preferredDevice = 'webgpu';
    } catch (_error) {
      preferredDevice = 'wasm';
    }
  }
  try {
    generator = await createPipeline(preferredDevice);
    activeDevice = preferredDevice;
  } catch (error) {
    if (preferredDevice !== 'webgpu') throw error;
    generator = await createPipeline('wasm');
    activeDevice = 'wasm';
  }
  self.postMessage({ type: 'ready', device: activeDevice });
  return generator;
}

self.addEventListener('message', async event => {
  const { type, messages } = event.data || {};
  if (type === 'dispose') {
    await generator?.dispose?.();
    generator = null;
    self.close();
    return;
  }
  if (type !== 'generate' || !Array.isArray(messages)) return;
  try {
    const pipeline = await ensureGenerator();
    const result = await pipeline(messages, {
      max_new_tokens: 32,
      do_sample: false,
      repetition_penalty: 1.08,
      return_full_text: false
    });
    self.postMessage({ type: 'result', text: extractGeneratedText(result), device: activeDevice });
  } catch (error) {
    self.postMessage({ type: 'error', message: error?.message || String(error) });
  }
});
