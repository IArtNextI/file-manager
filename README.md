# File Manager

## Functionality

- Supports cut-and-paste (`x` for cut, `v` for paste)
- Supports copy-and-paste (`c` for copy, `v` for paste)
- Adapts to the size of the window
- Supports plugins (more on this later)
- Highlights filenames based on their type
- Supports modes for hidden files (`h` to toggle)
- Written in pure C (no third-party libs)

## Build the project

From the root of the repository, run following commands:

```bash
mkdir build && cd build
cmake ..
make
```

## Plugins

Manager supports plugins to open different file types:

In order for it to work, you have create a folder named **file-manager-plugins** in the directory from which the executable is called.

On the event of **Enter** being pressed on a file with specified **extension**, manager looks for a **lib\<extension\>.so** file and looks for a defined **void open_file(const char* filename)** handle.

e.g. if **Enter** is pressed on a file called "a.txt", the required lib is **libtxt.so**

If the required handle is found, it is called with **filename** parameter being equal to the absolute path to the filename.


