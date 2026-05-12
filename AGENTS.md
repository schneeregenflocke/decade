# AGENTS.md

## wxWeakRef

https://docs.wxwidgets.org/3.2/classwx_weak_ref_3_01_t_01_4.html
https://docs.wxwidgets.org/3.2/classwx_trackable.html

## clang-tidy

```
clang-tidy -p build src/**/*.cpp src/**/*.hpp 2>&1 | tee build/clang-tidy.log
```
```
find . -regex '.*\.\(cpp\|hpp\|cc\|cxx\|h\)' -exec clang-format -style=file -i {} +
```