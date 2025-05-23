![Ocre logo](ocre_logo.jpg "Ocre")
# Ocre
[![Build](https://github.com/project-ocre/ocre-runtime/actions/workflows/build.yml/badge.svg)](https://github.com/project-ocre/ocre-runtime/actions/workflows/build.yml)
[![OpenSSF Best Practices](https://www.bestpractices.dev/projects/9691/badge)](https://www.bestpractices.dev/projects/9691)
[![License](https://img.shields.io/github/license/project-ocre/ocre-runtime?color=blue)](LICENSE)
[![slack](https://img.shields.io/badge/slack-ocre-brightgreen.svg?logo=slack)](https://lfedge.slack.com/archives/C07F190CC3X)
[![Stars](https://img.shields.io/github/stars/project-ocre/ocre-runtime?style=social)](Stars)

Ocre is a container runtime for constrained devices. It leverages [WebAssembly](https://www.webassembly.org) and [Zephyr](https://www.zephyrproject.org/) to support OCI-type application containers in a footprint up to 2,000 times smaller than traditional Linux-based container runtimes. Our mission is to modernize the embedded applications by making it as easy to develop and securely deploy apps on constrained edge devices as it is in the cloud.

## Getting Started 
This guide walks you through building and running Ocre on a simulated device using Zephyr's `native_sim` target. For instructions on building and flashing Ocre to physical hardware, please refer to our [documentation](https://docs.project-ocre.org/quickstart/firmware/hardware/).

The application in `./src/main.c` demonstrates basic Ocre runtime usage by writing a hello world application to flash and executing it.


1. **Install Dependencies and Zephyr SDK**

Complete the [Install dependencies](https://docs.zephyrproject.org/3.7.0/develop/getting_started/index.html#install-dependencies) and the [Install the Zephyr SDK](https://docs.zephyrproject.org/3.7.0/develop/getting_started/index.html#install-the-zephyr-sdk) sections for your host operating system from the Zephyr (v3.7.0) [Getting Started Guide](https://docs.zephyrproject.org/3.7.0/develop/getting_started/index.html#getting-started-guide). 

2. Create a Virtual Python Environment (`venv`)

```
mkdir runtime && cd runtime
python3 -m venv .venv
source .venv/bin/activate
```

**Note:** You may need to install the `python3-venv` package (or equivalent) on your host system beforehand.

3. **Install WEST**

Install the [west](https://docs.zephyrproject.org/latest/develop/west/index.html) CLI tool, which is needed to build, run and manage Zephyr applications.

```
pip install west
```

4. **Initialize the workspace**

This will checkout the Ocre runtime code and initalize the workspace.
```
west init -m git@github.com:project-ocre/ocre-runtime.git
```

5. **Update West**

Next, we need to update the workspace with the latest Zephyr and WASM Micro Runtime code.

```
west update
```

6. **Install Additional Zephyr (pip) requirements**

In order to build the Ocre runtime properly, you'll need to install a few remaining requirements for Zephyr.

```
pip install -r zephyr/scripts/requirements.txt
```

7. **Build the application**

The following will build the firmware for the *virtual*, `native_sim` target which will allow you to run the Ocre runtime on a simulated device, rather than a physical board.
```
west build -b native_sim ./application -d build -- -DMODULE_EXT_ROOT=`pwd`/application
```
8. **Run the application**

Run the following command:
```
./build/zephyr/zephyr.exe
```

---
## License
Distributed under the Apache License 2.0. See [LICENSE](https://github.com/project-ocre/ocre-runtime/blob/main/LICENSE) for more information.

---
## More Info
* **[Website](https://lfedge.org/projects/ocre/)**: For a high-level overview of the Ocre project, and its goals, visit our website.
* **[Docs](https://docs.project-ocre.org/)**: For more detailed information about the Ocre runtime, visit Ocre docs.
* **[Wiki](https://lf-edge.atlassian.net/wiki/spaces/OCRE/overview?homepageId=14909442)**: For a full project overview, FAQs, project roadmap, and governance information, visit the Ocre Wiki.
* **[Slack](https://lfedge.slack.com/archives/C07F190CC3X)**: If you need support or simply want to discuss all things Ocre, head on over to our Slack channel (`#ocre`) on LFEdge's Slack org.
* **[Mailing list](https://lists.lfedge.org/g/Ocre-TSC)**: Join the Ocre Technical Steering Committee (TSC) mailing list to stay up to date with important project announcements, discussions, and meetings.
