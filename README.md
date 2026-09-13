# applications

This repository is one part of the Zephyr workspace. The workspace also
contains the `common`, `proto`, and `python` repositories. Each repository has
its own git history.

This repository holds the firmware applications. Each subdirectory is one
application.

## How to build

Run `make` from the directory of the application. Each application includes
`common.mk` from the workspace root, which supplies the targets. `common.mk`
also sources the workspace environment and activates the build virtual
environment, thus no manual step comes first.

```bash
cd <app_name>
make BOARD=<board> build
make flash
make mon
```

Run `make help` for the full list of targets. Run `make appboards` for the
boards that the application supports.

Use the board identifier, not the name of the file in `boards/`. A board with
more than one CPU cluster needs the cluster name:

```bash
make BOARD=w55rp20_evb_pico build
make BOARD=esp32s3_matrix/esp32s3/procpu build
```

Refer to the workspace README for the workspace setup, and to the `common`
README for the boards and the modules.

## The structure of an application

An application has this layout. `led_demo` is the example:

```
led_demo/
├── CMakeLists.txt          Collects the conf fragments and the sources
├── Kconfig                 The application symbols
├── Makefile                Includes common.mk, which supplies the targets
├── prj.conf                The base configuration
├── README.md
├── .copier-answers.yml     Present when the generator wrote the application
├── boards/                 One conf and one overlay for each board
│   ├── w55rp20_evb_pico.conf
│   └── w55rp20_evb_pico.overlay
├── conf/                   Configuration fragments, by subject
│   ├── kernel.conf
│   ├── modules.conf        The modules that the application enables
│   ├── net.conf
│   └── ...
└── src/                    The application sources
    ├── main.c
    └── ...
```

Most applications hold a README that describes their own function.

### The configuration

Zephyr merges several files to build the configuration. `prj.conf` holds the
base. `CMakeLists.txt` then appends the fragments of `conf/` through
`EXTRA_CONF_FILE`, and Zephyr applies the files of `boards/` for the board
that you select.

The fragments divide the configuration by subject, thus you can read one
concern at a time. `conf/modules.conf` is the important one: it enables the
modules of the `common` repository. It lists the full Kconfig dependency
closure, because a symbol with an unmet dependency is dropped without an
error and the headers of the module then leave the include path.

A file under `boards/` uses the conf-file basename of the board, not the
board identifier. Thus `esp32s3_matrix/esp32s3/procpu` reads
`boards/esp32s3_matrix_procpu.conf`.

### The network type

An application that uses a network selects one type. `Kconfig` supplies the
`APP_NET_TYPE` choice, and `CMakeLists.txt` calls `app_net_type_resolve` from
the `common` repository to append the matching fragment: `conf/wifi.conf`,
`conf/serial_net.conf`, `conf/wired_eth_net.conf`, or `conf/usb_net.conf`.
`app_net_type_verify` then confirms that the merged configuration agrees with
the type that you selected.

## How to add an application: app_gen

Use `app_gen` from the `python` repository. It reads the Copier template in the
`common` repository, then writes the new application to this repository:

```bash
app_gen
```

`app_gen` runs a TUI. `app_gen new --help` gives the command line form.

An application that `app_gen` generates holds a `.copier-answers.yml` file.
`app_gen update <app>` applies later template changes to it. An application
without that file came from an earlier procedure, thus `app_gen update` cannot
change it.

## Note: The manifest-rev branch

west manages this repository through the workspace manifest. Thus west leaves
it on a `manifest-rev` branch, which you cannot commit to.

Switch the workspace repositories to `main` before you make changes:

```bash
./init_workspace.sh --main
```

`west update` returns each repository to `manifest-rev`. Thus you must run the
command again after each update. Refer to the workspace README.
