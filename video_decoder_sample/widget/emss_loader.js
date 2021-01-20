// Copyright (c) 2020 Samsung Electronics Inc.
// Licensed under the MIT license.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// eslint-disable-next-line no-var, no-unused-vars
var Module = undefined;

// Loads a WASM module. moduleScriptUrl should point to a JS loader script
// generated by Emscripten.
//
// This function loads modules with no MODULARIZE option.
function loadWasmModule(moduleScriptUrl) {
  return new Promise((resolve, reject) => {
    try {
      // This variable will be used by a loader script generated by Emscripten.
      Module = {
        onRuntimeInitialized: () => {
          console.info('WASM runtime initialized');
          resolve();
        },
        onAbort: () => {
          console.error('WASM runtime aborted');
          reject(new Error('WASM runtime aborted'));
        },
        onExit: (status) => {
          console.info(`WASM onExit called with status: ${status}`);
        },
        canvas: (function() {
          var canvas = document.getElementById('canvas');
          return canvas;
        })(),
      }; // Module

      const script = document.createElement('script');

      script.addEventListener('load', () => {
        console.info(`Loaded ${moduleScriptUrl}`);
      });

      script.addEventListener('error', (errorEvent) => {
        console.error(`Cannot load ${moduleScriptUrl}`);
        reject(new Error(
            `Cannot load ${moduleScriptUrl} error: ${errorEvent.message}`));
      });

      script.async = true;
      script.src = moduleScriptUrl;
      document.body.appendChild(script);
    } catch (err) {
      console.error(err);
      reject(new Error('Cannot instantiate WASM module!'));
    }
  });
} // loadWasmModule()

window.addEventListener('DOMContentLoaded', async () => {
  const wasmLoadingDiv = document.getElementById('wasm-loading');
  try {
    await loadWasmModule(
        'wasm_modules/VideoDecoderSampleModule/CurrentBin/VideoDecoderSampleModule.js');
  } catch (ex) {
    wasmLoadingDiv.innerHTML = '<p class="error">Cannot load WASM module.</p>';
    return;
  }

  const videoElement = document.getElementById('video-element');
  wasmLoadingDiv.classList.add('invisible');
  videoElement.classList.remove('invisible');
}); // DOMContentLoaded handler
