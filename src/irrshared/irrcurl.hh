#include "irrlicht.h"

irr::io::IReadFile* createAndOpen(irr::io::IFileSystem* fs,const wchar_t* url);
irr::io::IReadFile* createAndOpenURL(irr::io::IFileSystem* fs,const wchar_t* url);
irr::io::IReadFile* createAndOpen(irr::io::IFileSystem* fs,const char* url);
irr::io::IReadFile* createAndOpenURL(irr::io::IFileSystem* fs,const char* url);

