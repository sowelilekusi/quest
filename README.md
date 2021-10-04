# Quinn's Utterly Elegant Speedrun Timer

A speedrun timer for \*nix terminals

## Features

- Global hotkeys
- 24-bit terminal color
- Human readable JSON splits file
- Import splits from a Splits.io file
- High fps rendering

## Usage

Copy `examples/sample.json` somewhere, replace the sample information with
the names of your game, catagories and splits. Changing the number of
splits after the first use of the split file is currently unsupported.

### Default Keybinds
| Keys | Action                | Global |
| ---- | --------------------- | ------ |
| `R`  | Start                 | YES    |
| `F`  | Stop / Reset          | YES    |
| `E`  | Split                 | YES    |
| `G`  | Undo split            | YES    |
| `V`  | Skip split            | YES    |
| `Q`  | Close                 | NO     |
| `T`  | Toggle global hotkeys | YES    |
| `C`  | Toggle compact UI     | NO     |

Customisable hotkeys without editing the source code coming soon!
