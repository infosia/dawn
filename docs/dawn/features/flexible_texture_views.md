# Flexible Texture Views
In Compat Mode, a texture by default:
- Must be created with texture view dimension specified.
- Cannot use different views in the same draw call.
- Cannot have 2D view for a 2D array texture.

The `FlexibleTextureViews` feature removes the above restrictions.
