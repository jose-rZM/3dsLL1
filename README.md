# 🎮📚 3DS Grammar Homebrew

A small homebrew app for the Nintendo 3DS that lets you **define simple grammars**, **process them**, and **play around** with them using the virtual keyboard.

## ✨ What does it do?

- Lets you input a grammar right on your 3DS (multiline input supported!).
- Automatically detects the axiom (it's the production that ends with `$`).
- Processes productions like `A: a A`.
- All from your **Nintendo 3DS**. Because why not.

## 🎛️ Controls

- **A** → Add a new grammar
- **B** → Process the current grammar
- **Y** → Reset everything

## 📖 Input format

```text
S: a A $
A: b A
A: EPSILON
```
- Non-terminals: Uppercase (e.g. S, A, B)
- Terminals: Lowercase (e.g. a, b, c)
- EPSILON: Special keyword representing the empty string.
- $: End of input marker

## ▶️ How to run it
1. Clone the repo.
2. Copy the file `ll1brew.3dsx` into your `/3ds` folder on your SD card
3. Open the Homebrew Launcher on your 3ds
4. Have fun

You can also run the application in a 3ds emulator.
