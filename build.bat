xcopy .\src\platform\iodp_linux\iodp_serial_linux.h .\include\  /Y
xcopy .\src\platform\iodp_win\iodp_tcp_win.h .\include\  /Y
xcopy .\src\platform\iodp_qt\qiodp.h .\include\  /Y
xcopy .\src\eiodp.h .\include\  /Y
cd build
cmake -G "MinGW Makefiles" ..
make