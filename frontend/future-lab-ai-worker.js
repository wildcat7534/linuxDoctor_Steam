const MODEL_ID = 'onnx-community/gemma-3-270m-it-ONNX';
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
  const pipeline = await transformers.pipeline('text-generation', MODEL_ID, {
    dtype: 'fp16',
    device,
    progress_callback: progress => self.postMessage({ type: 'progress', progress })
  });
  if (!pipeline.tokenizer) {
    const modelRoot = new URL(`models/${MODEL_ID}/`, import.meta.url);
    const [tokenizerJSON, tokenizerConfig] = await Promise.all([
      fetch(new URL('tokenizer.json', modelRoot), { cache: 'no-store' }).then(response => response.json()),
      fetch(new URL('tokenizer_config.json', modelRoot), { cache: 'no-store' }).then(response => response.json())
    ]);
    pipeline.tokenizer = new transformers.GemmaTokenizer(tokenizerJSON, tokenizerConfig);
  }
  if (!pipeline.tokenizer._tokenizerConfig) {
    pipeline.tokenizer._tokenizerConfig = {
      add_bos_token: true,
      add_eos_token: false
    };
  }
  return pipeline;
}

async function ensureGenerator() {
  if (generator) return generator;
  transformers = await import(new URL(RUNTIME_MODULE, import.meta.url));
  const { env } = transformers;
  env.allowRemoteModels = false;
  env.allowLocalModels = true;
  env.localModelPath = new URL('models/', import.meta.url).href;
  // Every asset is already served from frontend/models. CacheStorage is not
  // available in every isolated Worker and is unnecessary for this local app.
  env.useBrowserCache = false;
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
  } catch (webgpuError) {
    if (preferredDevice !== 'webgpu') throw webgpuError;
    try {
      generator = await createPipeline('wasm');
      activeDevice = 'wasm';
    } catch (wasmError) {
      throw new Error(`WebGPU : ${webgpuError?.message || webgpuError}; WASM : ${wasmError?.message || wasmError}`);
    }
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
  if (type === 'load') {
    try {
      await ensureGenerator();
    } catch (error) {
      self.postMessage({ type: 'error', message: error?.message || String(error) });
    }
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
