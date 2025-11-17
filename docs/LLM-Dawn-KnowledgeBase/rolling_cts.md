# Rolling CTS

Notes and instructions for rolling CTS manually.

- Roll history: https://dawn-review.googlesource.com/q/file:third_party/webgpu-cts+-%22%5BDO+NOT+SUBMIT%5D%22

## Setup
One-time setup to authenticate `gcloud` before the CTS roller can be used:

- Install gcloud SDK
- `gcloud auth login`
- `gcloud auth application-default login`

## Rolling CTS
The CTS roll can take a long time (sometimes >15 hrs) so run it in a persistent session.

- `./tools/run cts roll`
