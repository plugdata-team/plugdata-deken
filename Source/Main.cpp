/*
 ==============================================================================
 
 This file contains the basic startup code for a JUCE application.
 
 ==============================================================================
 */

#include <JuceHeader.h>
#include "JSON.h"

// Struct with info about the deken package
struct PackageInfo {
    PackageInfo(String name, String author, String timestamp, String url, String description, String version, StringArray objects)
    {
        this->name = name;
        this->author = author;
        this->timestamp = timestamp;
        this->url = url;
        this->description = description;
        this->version = version;
        this->objects = objects;
        packageId = Base64::toBase64(name + "_" + version + "_" + timestamp + "_" + author);
    }
    
    String name, author, timestamp, url, description, version, packageId;
    StringArray objects;
};

// Array with package info to store the result of a search action in
using PackageList = std::unordered_map<String, Array<PackageInfo>>;

using namespace nlohmann;

static inline const String floatsize = String(32);
std::unique_ptr<WebInputStream> webstream;
ValueTree cacheState = ValueTree("cache");

StringArray getObjectInfo(String const& objectUrl)
{
    StringArray result;
    
    webstream = std::make_unique<WebInputStream>(URL("https://deken.puredata.info/info.json?url=" + objectUrl), false);
    webstream->connect(nullptr);
    
    // Read json result
    auto json = webstream->readString();
    
    
    // Parse outer JSON layer
    auto parsedJson = json::parse(json.toStdString());
    
    // Read json
    auto objects = (*((*(parsedJson["result"]["libraries"].begin())).begin())).at(0)["objects"];
    
    for (auto obj : objects) {
        result.add(obj["name"]);
    }
    
    return result;
}

//==============================================================================
int main (int argc, char* argv[])
{
    
    std::unordered_map<String, PackageList> packages;
    
    webstream = std::make_unique<WebInputStream>(URL("https://deken.puredata.info/search.json"), false);
    webstream->connect(nullptr);
    
    // Read json result
    auto json = webstream->readString();
    
    // JUCE json parsing unfortunately fails to parse deken's json...
    auto parsedJson = json::parse(json.toStdString());
    
    // Read json
    auto object = parsedJson["result"]["libraries"];
    
    // Valid result, go through the options
    for (auto const& versions : object) {
  
        // You can read object names with this JSON library...
        String name;
        
        // Loop through the different versions
        for (auto platforms : versions) {
            
            // Loop through architectures
            for (auto platform : platforms) {
                
                for (auto architecture : platform["archs"]) {
                    // Extract platform
                    String arch = architecture.is_null() ? "null" : architecture;
                    
                    // Extract info
                    if(name.isEmpty()) name = platform["name"];
                    String author = platform["author"];
                    String timestamp = platform["timestamp"];
                    
                    String description = platform["description"];
                    String url = platform["url"];
                    String version = platform["version"];
                    
                    StringArray objects = getObjectInfo(url);
                    
                    // Add valid option
                    packages[arch][name].add({ name, author, timestamp, url, description, version, objects });
                }
            }
        }
        
        // If there were any results, sort by alphabetically by timestamp to get latest version
        // The timestamp format is yyyy:mm::dd hh::mm::ss so this should work
        for(auto& [platform, os_packages] : packages)
        if (!os_packages[name].isEmpty()) {
            std::sort(os_packages[name].begin(), os_packages[name].end(), [](auto const& result1, auto const& result2) { return result1.timestamp.compare(result2.timestamp) > 0; });
        }
    }
    
    auto destfolder = File::getCurrentWorkingDirectory().getChildFile("result");

    auto xmlout = destfolder.getChildFile("xml");
    auto compressedout = destfolder.getChildFile("bin");
    
    xmlout.createDirectory();
    compressedout.createDirectory();
    
    for(auto& [platform, library] : packages) {
        ValueTree platformTree = ValueTree(Identifier(platform));
        
        for (auto& [name, package] : library) {
            
            if(!package.size()) continue;

            ValueTree pkgList = ValueTree("Packages");
            pkgList.setProperty("Name", name, nullptr);
            
            for(auto& version : package)
            {
                ValueTree pkgEntry = ValueTree("Package");
                pkgEntry.setProperty("ID", version.packageId, nullptr);
                pkgEntry.setProperty("Author", version.author, nullptr);
                pkgEntry.setProperty("Timestamp", version.timestamp, nullptr);
                pkgEntry.setProperty("Description", version.description, nullptr);
                pkgEntry.setProperty("Version", version.version, nullptr);
                pkgEntry.setProperty("URL", version.url, nullptr);
                
                ValueTree objects("Objects");
                for (auto& object : version.objects) {
                    auto objectTree = ValueTree("Object");
                    objectTree.setProperty("Name", object, nullptr);
                    objects.appendChild(objectTree, nullptr);
                }
                
                pkgEntry.appendChild(objectTree);

                pkgList.appendChild(pkgEntry, nullptr);
            }
            
            platformTree.appendChild(pkgList, nullptr);
        }
        
        // Readable xml output for debugging
        auto outfile = xmlout.getChildFile(platform + ".xml");
        outfile.create();
        outfile.replaceWithText(platformTree.toXmlString());
        
        // Compressed output for fast downloads
        auto compressedfile = compressedout.getChildFile(platform + ".bin");
        compressedfile.create();
        
        FileOutputStream stream(compressedfile);
        platformTree.writeToStream(stream);
        stream.flush();
        
    }    
     
    // ..your code goes here!
    
    webstream.reset(nullptr);
    
    return 0;
}
