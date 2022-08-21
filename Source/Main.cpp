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

/*
struct PackageManager : public ActionBroadcaster public ValueTree::Listener public DeletedAtShutdown {
    
    PackageManager()
    {
        cacheState.addListener(this);
        
        startThread(3);
    }
    
    ~PackageManager()
    {
        if (webstream)
            webstream->cancel();
        stopThread(500);
    }
    
    void update()
    {
        sendActionMessage("");
        startThread(3);
    }
    
    void run() override
    {
        // Continue on pipe errors
#ifndef _MSC_VER
        signal(SIGPIPE, SIG_IGN);
#endif
        allPackages = getAvailablePackages();
        sendActionMessage("");
    }
    
    
    
    
     PackageList readFromCache()
     {
     PackageList result;
     
     auto state = cacheState.getChildWithName("State");
     for (auto package : state) {
     auto name = package.getProperty("Name").toString();
     auto author = package.getProperty("Author").toString();
     auto timestamp = package.getProperty("Timestamp").toString();
     auto url = package.getProperty("URL").toString();
     auto description = package.getProperty("Description").toString();
     auto version = package.getProperty("Version").toString();
     StringArray objects;
     
     for (auto object : package.getChildWithName("Objects")) {
     objects.add(object.getProperty("Name").toString());
     }
     
     result.add(PackageInfo(name, author, timestamp, url, description, version, objects));
     }
     
     return result;
     }
    
    
    
    
     static bool checkArchitecture(String platform)
     {
     // Check OS
     if (platform.upToFirstOccurrenceOf("-", false, false) != os)
     return false;
     platform = platform.fromFirstOccurrenceOf("-", false, false);
     
     // Check floatsize
     if (platform.fromLastOccurrenceOf("-", false, false) != floatsize)
     return false;
     platform = platform.upToLastOccurrenceOf("-", false, false);
     
     if (machine.contains(platform))
     return true;
     
     return false;
     }
    
    PackageList allPackages;
    
    inline static File filesystem = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("PlugData").getChildFile("Library").getChildFile("Deken");
    
    ValueTree cacheState = ValueTree("cache");
    
    std::unique_ptr<WebInputStream> webstream;
    
    static inline const String floatsize = String(32);
    static inline const String os =
#if JUCE_LINUX
    "Linux"
#elif JUCE_MAC
    "Darwin"
#elif JUCE_WINDOWS
    "Windows"
    // PlugData has no official BSD support and testing, but for completeness:
#elif defined __FreeBSD__
    "FreeBSD"
#elif defined __NetBSD__
    "NetBSD"
#elif defined __OpenBSD__
    "OpenBSD"
#else
#    if defined(__GNUC__)
#        warning unknown OS
#    endif
    0
#endif
    ;
    
    static inline const StringArray machine =
#if defined(__x86_64__) || defined(__amd64__) || defined(_M_X64) || defined(_M_AMD64)
    { "amd64", "x86_64" }
#elif defined(__i386__) || defined(__i486__) || defined(__i586__) || defined(__i686__) || defined(_M_IX86)
    { "i386", "i686", "i586" }
#elif defined(__ppc__)
    { "ppc", "PowerPC" }
#elif defined(__aarch64__)
    { "arm64" }
#elif __ARM_ARCH == 6 || defined(__ARM_ARCH_6__)
    { "armv6", "armv6l", "arm" }
#elif __ARM_ARCH == 7 || defined(__ARM_ARCH_7__)
    { "armv7l", "armv7", "armv6l", "armv6", "arm" }
#else
#    if defined(__GNUC__)
#        warning unknown architecture
#    endif
    {}
#endif
    ;
    
}; */

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
                    
                    std::cout << arch << std::endl;
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
        
        // Speed it up for testing
        if(packages.size() > 5) {
            break;
        }
    }
    
    auto xmlout = File::getCurrentWorkingDirectory().getChildFile("xml");
    auto compressedout = File::getCurrentWorkingDirectory().getChildFile("bin");
    
    for(auto& [platform, library] : packages) {
        ValueTree platformTree = ValueTree(Identifier(platform));
        
        for (auto& [name, package] : library) {
            
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
                
                pkgList.appendChild(pkgEntry, nullptr);
            }
            
            platformTree.appendChild(pkgList, nullptr);
        }
        
        auto outfile = xmlout.getChildFile(platform + ".xml");
        outfile.create();
        
        outfile.replaceWithText(platformTree.toXmlString());
    }

    
    
    
    std::cout << packages.size() << std::endl;
    
    // ..your code goes here!
    
    
    return 0;
}
