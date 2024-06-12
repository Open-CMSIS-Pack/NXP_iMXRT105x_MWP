[![Version](https://img.shields.io/github/v/release/Open-CMSIS-Pack/NXP_iMXRT105x_MWP)](https://github.com/Open-CMSIS-Pack/NXP_iMXRT105x_MWP/releases/latest)

# NXP_iMXRT105x_MWP

This is the development repository for the **NXP i.MXRT1051/1052 Device Series Middleware Pack (MWP)** - a CMSIS software pack that is designed to work with all compiler toolchains (Arm Compiler, GCC, IAR, LLVM). It is released as [CMSIS software pack](https://www.keil.arm.com/packs/imxrt105x_mwp-keil) and therefore accessible by CMSIS-Pack enabled software development tools.

This MWP provides [CMSIS-Drivers](https://arm-software.github.io/CMSIS_6/latest/Driver) for i.MXRT1051/1052 Device Series which can be used by Middleware.

> **Note:** This is currently Work in Progress. Final release is expected in Q3'2024.

## Repository top-level structure

Directory                   | Description
:---------------------------|:--------------
[.github/workflows](https://github.com/Open-CMSIS-Pack/NXP_iMXRT105x_MWP/tree/main/.github/workflows)  | [GitHub Actions](#github-actions).
[CMSIS/Driver](https://github.com/Open-CMSIS-Pack/NXP_iMXRT105x_MWP/tree/main/CMSIS/Driver)            | CMSIS-Drivers for Ethernet MAC, FLEXCAN, MCI, USB Host and Device.
[Documentation](https://github.com/Open-CMSIS-Pack/NXP_iMXRT105x_MWP/tree/main/Documentation)          | Doxygen source of the [documentation](https://open-cmsis-pack.github.io/NXP_iMXRT105x_MWP/latest/index.html).
[Scripts](https://github.com/Open-CMSIS-Pack/NXP_iMXRT105x_MWP/tree/main/Scripts)                      | Updated linker scripts for using Event Recorder and debugger scripts.
[SDK](https://github.com/Open-CMSIS-Pack/NXP_iMXRT105x_MWP/tree/main/SDK)                              | Enhancements for NXP SDK.
[Templates](https://github.com/Open-CMSIS-Pack/NXP_iMXRT105x_MWP/tree/main/Templates)                  | Various source code templates.

## Generate software pack

The software pack is generated using bash shell scripts.

- `./gen_pack.sh` based on [Open-CMSIS-Pack/gen-pack](
https://github.com/Open-CMSIS-Pack/gen-pack) generates the software pack. Run this script locally with:

      NXP_iMXRT105x_MWP $ ./gen_pack.sh

### GitHub Actions

The repository uses GitHub Actions to generate the pack and publish documentation examples:

- `.github/workflows/pack.yml` based on [Open-CMSIS-Pack/gen-pack-action](https://github.com/Open-CMSIS-Pack/gen-pack-action) generates pack using the [Generate software pack](#generate-software-pack) scripts.
- `.github/workflows/gh-pages.yml` publishes the documentation to [open-cmsis-pack.github.io/NXP_iMXRT105x_MWP](https://open-cmsis-pack.github.io/NXP_iMXRT105x_MWP/latest/index.html).

## License

The MWP is licensed under [![License](https://img.shields.io/github/license/Open-CMSIS-Pack/NXP_iMXRT105x_MWP?label)](https://github.com/Open-CMSIS-Pack/NXP_iMXRT105x_MWP/blob/main/CMSIS/LICENSE).

## Issues

Please feel free to raise an [issue on GitHub](https://github.com/Open-CMSIS-Pack/NXP_iMXRT105x_MWP/issues)
to report misbehavior (i.e. bugs) or start discussions about enhancements. This
is your best way to interact directly with the maintenance team and the community.
We encourage you to append implementation suggestions as this helps to decrease the
workload of the maintenance team.
