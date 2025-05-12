import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { createRequire } from 'node:module';
import { describe, it, before, after } from 'node:test';

const require = createRequire(import.meta.url);
    
const __dirname = dirname(fileURLToPath(import.meta.url));
const dawnNodePath = join(__dirname, '..', '..', '..', 'out', 'active', 'dawn.node');
const { create, globals } = require(dawnNodePath);

Object.assign(globalThis, globals);

const assert = {
  ok(cond, msg) {
    if (typeof cond === 'function') {
      assert.ok(cond(), msg ?? cond.toString());
    } else if (!cond) {
      throw new Error(msg ?? '');
    }
  }
};

describe('tests', () => {
  let device;

  before(async () => {
    const navigator = { gpu: create([]) };
    const adapter = await navigator.gpu.requestAdapter();
    if (!adapter) {
      return;
    }

    device = await adapter.requestDevice({label: "test-device"});
  });

  after(async () => {
    device.destroy();

    await new Promise(r => setTimeout(r, 1000));
  })

  describe('device tests', () => {

    it('basic device tests', () => {
      assert.ok(device.adapterInfo.description !== undefined);
      assert.ok(device.label === 'test-device');
      assert.ok(() => device instanceof EventTarget);
    });

    it('can not construct device', () => {
      // check we can't make things we're not supposed to be able to.
      let err;
      try {
        const gd = new GPUDevice();
      } catch (e) {
        err = e;
      }
      assert.ok(() => err instanceof TypeError);
    });

  });

  describe('error tests', () => {

    it('basic error tests', () => {
      const e = new GPUValidationError('test1');
      assert.ok(() => e instanceof GPUError);
      assert.ok(() => e.message === 'test1');
    });

  });

  describe('event tests', () => {

    it('can pass Event through device', async () => {
      const e = await new Promise(resolve => {
        device.addEventListener('uncapturedFoo', resolve);
        device.dispatchEvent(new Event('uncapturedFoo', { error: { message: 'yo (1)!'} } ));
      });
      assert.ok(!!e, 'Event worked');
    });

    it('can pass CustomEvent through device', async () => {
      const e = await new Promise(resolve => {
        device.addEventListener('uncapturedEvent', resolve, { once: true });
        device.dispatchEvent(new CustomEvent('uncapturedEvent', { detail: {error: { message: 'yo!'} } }));
      });
      assert.ok(e?.detail.error.message === 'yo!', 'CustomEvent worked');
    });

    it('can pass extended Event through device', async () => {
      class MyEvent extends Event {
        constructor(type, msg) {
          super(type);
          this.message = msg;
        }
      }
      const target = new EventTarget();
      const e = await new Promise(resolve => {
        target.addEventListener('custom', resolve, { once: true });
        target.dispatchEvent(new MyEvent('custom', 'hello'));
      });
      assert.ok(() => e.message === 'hello');
    });

    it('can pass dispatch user create GPUUncapturedErrorEvent', async () => {
      const gEvent = new GPUUncapturedErrorEvent(
        'uncapturederror',
        { error: new GPUValidationError('test2') }
      );

      assert.ok(() => gEvent instanceof Event);
      assert.ok(() => gEvent.error.message === 'test2');

      const e = await new Promise(resolve => {
        device.addEventListener('uncapturederror', resolve, { once: true } );
        device.dispatchEvent(gEvent);
      });
      assert.ok(e?.error.message === 'test2');
    });

    it('receives uncaptured events', async () => {
      const e = await new Promise(resolve => {
        device.addEventListener('uncapturederror', (e) => {
          e.preventDefault();
          resolve(e);
        }, { once: true, passive: false });
        const texture = device.createTexture({
          format: 'rgba8unorm',
          usage: GPUTextureUsage.TEXTURE_BINDING,
          size: [device.limits.maxTextureDimension2D + 1, 1],
        });
        texture.destroy();
      });
      assert.ok(() => e?.error.message.includes('maxTextureDimension2D'));
    });

  });

});
