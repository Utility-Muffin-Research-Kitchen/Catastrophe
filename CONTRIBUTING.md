# Contributing to Catastrophe

Thank you for your interest in contributing to Catastrophe.

## Getting Started

1. Fork and clone the repository
2. Install prerequisites (see [Getting Started](docs/GETTING_STARTED.md))
3. Build and run the examples on any supported desktop platform:
   ```bash
   make native
   make run-native-demo
   ```

## Code Style

- **C11** standard
- Public API functions: `cat_` prefix (e.g., `cat_draw_text`)
- Internal functions: `cat__` double-underscore prefix (e.g., `cat__render_pill`)
- Constants and macros: `CAT_` prefix, uppercase (e.g., `CAT_OK`, `CAT_DS()`)
- Enum values: `CAT_` prefix, uppercase with underscores (e.g., `CAT_BTN_START`)
- Indent with 4 spaces, no tabs
- Brace style: opening brace on the same line
- Label strings in `cat_footer_item` and `cat__button_names[]` are UPPERCASE by convention

## Project Structure

```
include/catastrophe.h          Core library (single header)
include/catastrophe_widgets.h  Widget library (single header)
examples/                     Example applications
docs/                         Documentation
res/                          Fonts and resources
ports/                        Platform-specific Makefiles
```

Both headers are **stb-style single-file libraries**. Define `CAT_IMPLEMENTATION` / `CAT_WIDGETS_IMPLEMENTATION` in exactly one `.c` file.

## Submitting Changes

1. Create a feature branch from `main`
2. Keep commits focused and descriptive
3. Test on at least one desktop platform (`make native && make run-native-demo`, or the equivalent `make mac`, `make linux`, or `make windows` target)
4. Test on at least one supported device
5. Open a pull request against `main` with a clear description of what and why

## Reporting Issues

- Use GitHub Issues
- Include platform, build target, and steps to reproduce
- Attach log output if applicable (`cat_log` writes to the configured log path)

## License

By contributing, you agree that your contributions will be licensed under the [MIT License](LICENSE).
