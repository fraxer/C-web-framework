#ifndef __MODULELOADER__
#define __MODULELOADER__

namespace moduleLoader {

int init(int, char*[]);

int reload();

int initModules();

int initModule(int (*)(), int (*)());

int parseLog();

int parseRoutes();

int parseDatabase();

int parseOpenssl();

int parseMimetype();

int parseThread();

int parseYookassa();

int parseS3();

int parseMail();

int parseEncryption();

} // namespace

#endif