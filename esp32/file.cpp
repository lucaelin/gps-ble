#include "file.h"

bool isGpsLogFile( const char *string )
{
  string = strrchr(string, '.');

  if( string != NULL )
    return ( strcmp(string, ".BIN") == 0 );

  return false;
}

File getGPSLog() {
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
    Serial.println(file.name());
    if(!file.isDirectory() && isGpsLogFile(file.name())) {
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

void readGPSLog(WiFiClient* client) {
  File root = FFat.open("/");
  if(!root){
    Serial.println("- failed to open directory");
    return;
  }

  File file = root.openNextFile();
  while(file){
    if(!file.isDirectory() && isGpsLogFile(file.name())) {
      Serial.print("Reading file: ");
      Serial.println(file.name());
      // read file
      while (file.available()) client->write(file.read());
    }

    file.close();
    file = root.openNextFile();
  }

  root.close();
}

uint32_t getGPSLogSize() {
  File root = FFat.open("/");
  if(!root){
    Serial.println("- failed to open directory");
    return 0;
  }

  uint32_t total = 0;
  File file = root.openNextFile();
  while(file){
    if(!file.isDirectory() && isGpsLogFile(file.name())) {
      Serial.print("Scanning file: ");
      Serial.println(file.name());
      total += file.size();
    }

    file.close();
    file = root.openNextFile();
  }

  root.close();

  return total;
}

bool deleteOldestFile() {
  File root = FFat.open("/");
  if(!root){
    Serial.println("- failed to open directory");
    return false;
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

  Serial.println("Found oldest file: ");
  Serial.println(name);

  if(FFat.remove(name)){
      Serial.println("- file deleted");
      return true;
  } else {
      Serial.println("- failed to delete");
      return false;
  }
}

void listAllFiles() {
  File root = FFat.open("/");
  if(!root){
    Serial.println("- failed to open directory");
    return;
  }

  File file = root.openNextFile();
  if (!file) Serial.println("No files.");
  while(file){
    if(!file.isDirectory()) {
      Serial.print(file.name());
      Serial.print(" (");
      Serial.print((int) file.size());
      Serial.println(")");
    }

    file.close();
    file = root.openNextFile();
  }

  root.close();
}
