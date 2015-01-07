#include "irrcurl.hh"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef USE_CURL
#include <curl/curl.h>

struct MemoryStruct {
  char *memory;
  size_t size;
};

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  mem->memory = (char*)realloc(mem->memory, mem->size + realsize + 1);
  if (mem->memory == NULL) {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    exit(EXIT_FAILURE);
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}


irr::io::IReadFile* createAndOpenURL(irr::io::IFileSystem* fs,const char* url){
  CURL *curl_handle;

  struct MemoryStruct chunk;

  chunk.memory = (char*)malloc(1);  /* will be grown as needed by the realloc above */
  chunk.size = 0;    /* no data at this point */

  curl_global_init(CURL_GLOBAL_ALL);

  /* init the curl session */
  curl_handle = curl_easy_init();

  /* specify URL to get */
  curl_easy_setopt(curl_handle, CURLOPT_URL, url);

  /* send all data to this function  */
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

  /* we pass our 'chunk' struct to the callback function */
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

  /* some servers don't like requests that are made without a user-agent
     field, so we provide one */
  curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

  /* get it! */
  curl_easy_perform(curl_handle);

  /* cleanup curl stuff */
  curl_easy_cleanup(curl_handle);

  irr::io::IReadFile* file;

  if(chunk.memory){
    char* data=new char[chunk.size];

    memcpy(data, chunk.memory, chunk.size);

    file=fs->createMemoryReadFile(data,chunk.size,url,true);

    free(chunk.memory);
  }
  /* we're done with libcurl, so clean it up */
  curl_global_cleanup();

  return file;
}
#else
irr::io::IReadFile* createAndOpenURL(irr::io::IFileSystem* fs,const char* url){
  return 0;
}
#endif

irr::io::IReadFile* createAndOpen(irr::io::IFileSystem* fs,const char* url){
  if(!url)
    return 0;
  const char* c=url;
  bool isurl=false;
  while((!isurl)&&*c!=0&&*c!='/'&&*c!='\\'){
    if(*c==':' && *(c+1)=='/' && *(c+2)=='/')
      isurl=true;
  }
  if(isurl)
    return createAndOpenURL(fs,url);
  else
    return fs->createAndOpenFile(url);
}
