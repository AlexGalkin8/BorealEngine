# 008 — Handle command-line arguments

Status: implemented.

## Outcome

Add a small `CommandLineHandler` class that receives the arguments already
tokenized by the platform entry point. The Windows sample currently calls
`CommandLineToArgvW` and passes the raw vector to `Engine::create`, where
argument handling is marked as TODO.

## Plan

1. Keep Windows tokenization at the sample entry point. Build one
   `CommandLineHandler` object from the resulting wide strings and handle the
   executable name separately from the application arguments.
2. Provide flag lookup, lookup for options that take a value, and a way to
   forward arguments that a startup feature has not consumed. Preserve the
   original order and values, including paths with spaces.
3. Use the parsed object for startup decisions before creating the engine.
   Resolve the TODO in `Engine::create` by updating its interface to reflect
   whether it still needs startup options.

## Acceptance criteria

- The sample starts normally with an empty argument list.
- The parser distinguishes a missing option from an option with a missing
  value. Startup reports a clear diagnostic for a malformed known option.
- A path with spaces arrives as one option value, and forwarded arguments
  preserve their order and text.

## Implementation

The sample calls `Engine::create()`, which reads the process arguments through
`engine::CommandLineHandler::fromProcess()` and parses them into
`CommandLineHandler::StartupOptions` before initializing the engine.
The default settings file is `data/settings.json`.

Flags and options are consumed before `--`. Repeated flags are consumed
together, and duplicate value options are reported. Arguments left after
consumption remain in their original order, including the `--` delimiter.

## Verification

Debug and Release x64 builds passed. The Debug sample passed the resize smoke
check with default settings and with a settings file in a non-ASCII path
containing spaces. The window title confirmed that the selected file was read.
Release reported a missing `--settings` value and exited before creating the
engine. A parser probe verified flags, forwarding, the option delimiter,
Unicode path values, and missing or duplicate options.

## Continuation

[RenderDoc startup](009-renderdoc-launch.md) will use the class for its launch
flags.
