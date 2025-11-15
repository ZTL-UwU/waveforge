# Level Format Documentation

This document describes the format used for level files in the Waveforge game. Level maps are represented as images, where specific pixel colors correspond to different pixel elements and structures within the game world. An additional metadata JSON file accompanies each level image to provide further configuration details.

## Level Map Image

The level map is stored as an image file (e.g., PNG) where each pixel's color defines the type of element or structure at that location. The image must be in a format compatible with SFML's `sf::Image` class and support RGBA colors. We recommend using PNG format. A complete list of theoretically supported formats includes:

- PNG: recommended, works properly
- TGA: likely to work properly
- PSD: unlikely to work properly
- HDR: likely to work properly
- PIC (Softimage PIC): must be of RGBA type

### Pixel Element Colors

Each pixel color in the level map corresponds to a specific pixel element type. The following table lists the colors and their associated pixel element types. All colors are specified in hexadecimal RGBA format.

| Color (Hex)              | Pixel Element Type |
|--------------------------|--------------------|
| `#00000000`              | Air                |
| `#40a4dfff`              | Water              |
| `#556b2fff`              | Oil                |
| `#7f7f7fff`              | Stone              |
| `#745a36ff` or `#4c3d26ff` | Wood               |
| `#b87333ff`              | Copper             |
| `#dacfa3ff` or `#c6ae71ff` | Sand               |

### Structure Marker Colors

Structures within the level are indicated by specific marker colors. The color marker defines the type of structure to be placed at that pixel location. The placement will replace the pixel element at that location with the corresponding structure. The marker pixel defines the top-left corner of the structure.

Some structures have a facing direction. A pixel of color `#ff000028` (semi-transparent red) indicates the facing direction of the structure. More specifically, let the pixel at `(x, y)` be the structure marker pixel. Then the location of the facing marker pixel is determined as follows:

| Facing Direction | Facing Marker Pixel Location          |
|------------------|---------------------------------------|
| North (Up)       | No facing marker pixel needed         |
| East (Right)     | `(x + 1, y)`                          |
| South (Down)     | `(x + 1, y + 1)`                      |
| West (Left)      | `(x, y + 1)`                          |

The following table lists the structure marker colors and their associated structures. All structure marker colors have an alpha value of `e7` (231 decimal), so the alpha channel is omitted in the table below.

| Color (Hex) | Structure Type  | Facing Required |
|-------------|-----------------|-----------------|
| `#33ffb8e7` | Laser Emitter  | Yes             |
| `#f0229fe7` | Pressure Plate | No              |

### Duck & Checkpoint Markers

The level map must contain exactly one duck marker and exactly one checkpoint marker. These markers are used to define the starting position of the player and the level's checkpoint, respectively. The following table lists the colors for these markers. Both marker colors have an alpha value of `e7` (231 decimal), so the alpha channel is omitted in the table below.

| Color (Hex) | Marker Type       |
|-------------|-------------------|
| `#fac82ee7` | Duck Marker       |
| `#59f1ffe7` | Checkpoint Marker |

## Metadata File

Each level image is accompanied by a metadata JSON file that provides additional configuration details for the level. The format of the metadata file is as follows:

```json
{
  "format": 1,
  "map": "level/simple-intro/map",
  "metadata": {
    "level_name": "Simple Intro",
    "description": "A basic introduction level",
    "author": "Developer"
  },
  "items": [
    {
      "id": "water_brush",
      "amount": 1
    }
  ]
}
```

Note: JSON does not support comments. The example above uses placeholder values for illustration.

### Items

The `items` array in the metadata file specifies the starting items available to the player in the level. Each item is represented by an object containing the following fields:

- `id`: A string representing the item ID.
- `amount`: An integer representing the quantity of the item.

Currently, the following item IDs are supported:

| Item ID       | Description                     |
|---------------|---------------------------------|
| `water_brush` | A brush that places water pixels |

## Level Loading

The level must be specified in the asset manifest under the `sequence` section. Each level usually consists of two entries: one for the level map image loading and another for the metadata JSON file loading. An example entry in the asset manifest is as follows:

```json
{
  "id": "level/simple-intro/map",
  "type": "image",
  "file": "levels/simple_intro.png",
  "description": "Loading \"simple intro\" level prototype image"
},
{
  "id": "level/simple-intro",
  "type": "level-metadata",
  "file": "levels/simple_intro.json",
  "description": "Loading \"simple intro\" level metadata"
}
```

Note: Correspondingly, the `map` field in the metadata JSON file should reference the level map image asset ID, e.g., `level/simple-intro/map` in the example above.
