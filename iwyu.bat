SET IWYU_PATH="..\Include What You Use\include-what-you-use"
SET IWYU=%IWYU_PATH%\iwyu_tool.py
if exist %IWYU% (
	python.exe %IWYU% -j 5 -v -p .\compile_commands.json -o clang-warning -- -Xiwyu --mapping_file=vs2022cpp20.imp --include=stdafx.h -D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH
)
