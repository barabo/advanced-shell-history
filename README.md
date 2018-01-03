# Advanced Shell History
### https://github.com/barabo/advanced-shell-history

Save your command line history in a sqlite3 DB!

## Features
  * Retains extra command details:
    * exit code
    * start and stop times (and command duration)
    * current working directory of the command
    * session details such as tty, pid, ppid, ssh connection details
  * works with multiple shells (zsh and bash)
  * saves history into a sqlite3 database
  * provides a convenient tool to query the history database.

## Build Instructions
```
  make build_c       # Builds the C++ version
  make build_python  # Builds the Python version
  make build         # Builds both versions
```

## Notes
  * A full build of the code from source depends on the following commands
    or utilities existing on your build system:
      - awk
      - bash
      - curl
      - flex (optional: only needed if rebuilding `queries.cpp` from `queries.l`)
      - g++ and gcc
      - make
      - sed
      - tar
      - unzip
  * Also recommended is `sqlite3` for interacting with the `history.db`.
  * In OSX, you will need to install xcode to compile the C++ version:
      http://developer.apple.com/xcode/

## Install
```{sh}
  make install_c       # Installs the C++ version
  make install_python  # Installs the Python version
  make install         # Installs both versions
```

## Configure
You will need to source a couple files in order for this to work by default.

First, source the configuration file in `/usr/local/etc/advanced-shell-history/config`.
This will define several environment variables that are needed for the logger and the
query app.

Next, export the `ASH_CFG_SYSTEM_QUERY_FILE` if you would like to use the systemwide
queries!  Without this exported line, the `ash_query` command will not know where to find
the queries needed to extract from the history database.

Finally, you must source the shell code for your shell.  For example, `bash` users source
`/usr/local/lib/advanced_shell_history/sh/bash` and `zsh` users source `/usr/local/lib/advanced_shell_history/sh/zsh`.

Here's a snippet you can paste into your `~/.bashrc` file:
```{sh}
  source /usr/local/etc/advanced-shell-history/config
  export ASH_CFG_SYSTEM_QUERY_FILE
  source /usr/local/lib/advanced_shell_history/sh/bash
```

## Uninstall
```
  make uninstall
```

## How It Works
This program operates on the assumption that your shell gives you the ability
to execute a custom operation before a prompt is redrawn on your terminal.

Bash provides the `PROMPT_COMMAND` environment variable while zsh gives us the
`precmd` environment function.  Both are repurposed by this program to log the
previous command into a local database before each new prompt is displayed.

Many people have never heard of this feature or found a great use for it.
However, if you currently DO use this feature, you can still use this program.
The code renames your existing hook and invokes it after saving the previous
command.

### Diagram
![Command Process Diagram](https://raw.githubusercontent.com/barabo/advanced-shell-history/master/docs/diagram.png)

## Caveats
  * this is not meant to be a security auditing tool - it's for user
    convenience and meant to enhance shell builtin history.
  * for bash users, this overrides your `PROMPT_COMMAND` and changes the 
    default options of your builtin history.  Hopefully both are an 
    improvement.
  * for zsh users, this overrides your `precmd` function.  The shell script
    attempts to rename your previous `precmd` function and continue to hook
    into it.
  * all users will notice that the `PIPESTATUS` / `pipestatus` variables have
    been renamed to `PIPEST_ASH` / `pipest_ash`.  Because these variables are transient
    and logged, they cannot be easily restored.  Instead they are copied.
  * when you build this, the Makefile may download a version of sqlite3
    (if it's not already included in the tarball or the svn repo).
  * this potentially changes your normal shell history settings to enable
    options necessary for the magic to work.

## Bugs
  * The C++ version does not compile in OSX because the CLOCK_MONOTONIC symbol is undefined.
    * Use the python version for now.
  * Doesn't capture exec'ed commands: example: `exec rm /tmp/foo # is lost`
  * The python version is about 10x slower than the C++ version.

### Author
carl.anderson@gmail.com (Carl Anderson)

### Updated
2017-03-16
