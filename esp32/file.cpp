#include "file.h"

bool isGpsLogFile( const char *string )
{
  string = strrchr(string, '.');

  if( string != NULL )
    return ( strcmp(string, ".BIN") == 0 );

  return false;
}

File getLatestFile() {
  File root = FFat.open("/");
  if(!root){
    Serial.println("- failed to open directory");
    return root;
  }

  bool found = false;
  File file = root.openNextFile();
  const char *name = "/";
  while(file){
    Serial.print("Found file: ");
    Serial.print(strcmp(file.name(), name));
    Serial.println(file.name());
    if(!file.isDirectory() && isGpsLogFile(file.name())){
      if(!found || strcmp(file.name(), name) > 0) {
        name = file.name();
        found = true;
      }
    }

    file.close();
    file = root.openNextFile();
  }

  root.close();
  
  Serial.println("Found latest file: ");
  Serial.println(name);
  return FFat.open(name);
}

void deleteOldestFile() {
  File root = FFat.open("/");
  if(!root){
    Serial.println("- failed to open directory");
    return;
  }

  bool found = false;
  File file = root.openNextFile();
  const char *name = "/";
  while(file){
    Serial.print("Found file: ");
    Serial.print(strcmp(file.name(), name));
    Serial.println(file.name());
    if(!file.isDirectory() && isGpsLogFile(file.name())){
      if(!found || strcmp(file.name(), name) < 0) {
        name = file.name();
        found = true;
      }
    }

    file.close();
    file = root.openNextFile();
  }

  root.close();
  
  Serial.println("Found latest file: ");
  Serial.println(name);
  
  if(FFat.remove(name)){
      Serial.println("- file deleted");
  } else {
      Serial.println("- failed to delete");
  }
}
