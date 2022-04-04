#include <kvdb/kvdbManager.hpp>

#include <assert.h>
#include <fstream>
#include <iostream>
#include <set>
#include <unordered_map>
#include <exception>
#include <filesystem>

#include <fmt/format.h>
#include <logging/logging.hpp>
#include <utils/stringUtils.hpp>

#include <kvdb/kvdb.hpp>

static KVDB invalidDB = KVDB();

// TODO Change Function Helpers tests initialization too.
KVDBManager* KVDBManager::mInstance = nullptr;
bool KVDBManager::init(const std::string& DbFolder)
{
    if (mInstance) {
        return false;
    }
    if (DbFolder.empty()) {
        return false;
    }
    std::filesystem::create_directories(DbFolder); // TODO Remove this whe Engine is integrated in Wazuh installation
    mInstance = new KVDBManager(DbFolder);
    return true;
}

KVDBManager& KVDBManager::get()
{
    if (!mInstance) {
        throw std::logic_error("KVDBManager isn't initialized");
    }
    return *mInstance;
}

KVDBManager::KVDBManager(const std::string& DbFolder)
{
    mDbFolder = DbFolder;

    static const std::set<std::string> INTERNALFILES = {
        "legacy", "LOG", "LOG.old", "LOCK"};
    for (const auto &dbFile : std::filesystem::directory_iterator(mDbFolder))
    {
        if (INTERNALFILES.find(dbFile.path().filename()) == INTERNALFILES.end())
        {
            if (!addDB(dbFile.path().stem(), mDbFolder))
            {
                WAZUH_LOG_DEBUG("Couldn't add new DB: [{}]", dbFile.path().stem().c_str());
            }
        }
    }

    if (std::filesystem::exists(mDbFolder + "legacy"))
    {
        for (const auto &cdbfile :
             std::filesystem::directory_iterator(mDbFolder + "legacy"))
        { // Read it from the config file?
            if (cdbfile.is_regular_file())
            {
                if (createDBfromCDB(cdbfile.path(), false))
                {
                    // TODO Remove files once synced
                    // std::filesystem::remove(cdbfile.path())
                }
            }
        }
    }
}

KVDBManager::~KVDBManager()
{
    m_availableKVDBs.clear();
}

bool KVDBManager::createDB(const std::string &name, bool overwrite)
{
    rocksdb::Options createOptions;
    createOptions.IncreaseParallelism();
    createOptions.OptimizeLevelStyleCompaction();
    createOptions.create_if_missing = true;
    createOptions.error_if_exists = !overwrite;

    rocksdb::DB *db;
    rocksdb::Status s = rocksdb::DB::Open(createOptions, mDbFolder + name, &db);
    if (!s.ok())
    {
        WAZUH_LOG_ERROR("couldn't open db file, error: [{}]", s.ToString());
        return false;
    }
    s = db->Close(); // TODO: We can avoid this unnecessary close making a more
                     // complex KVDB constructor that creates DBs
                     // or if receives a CFHandler and a CFDescriptor vector.
    if (!s.ok())
    {
        WAZUH_LOG_ERROR("couldn't close db file, error: [{}]", s.ToString());
        return false;
    }

    return addDB(name, mDbFolder);
}

bool KVDBManager::createDBfromCDB(const std::filesystem::path &path,
                                  bool overwrite)
{
    std::ifstream CDBfile(path);
    if (!CDBfile.is_open())
    {
        //_ERROR("Can't open CDB file");
        return false;
    }

    if (!createDB(path.stem(), overwrite))
    {
        WAZUH_LOG_ERROR("Couldn't create DB [{}]", path.stem().c_str());
        return false;
    }
    KVDB &kvdb = getDB(path.stem());
    if (kvdb.getState() != KVDB::State::Open)
    {
        WAZUH_LOG_ERROR("Created DB [{}] is not open", path.stem().c_str());
        return false;
    }

    std::string line;
    while (getline(CDBfile, line))
    {
        line.erase(remove_if(line.begin(), line.end(), isspace), line.end());
        auto KV = utils::string::split(line, ':');
        if (!KV.empty() && !KV.at(0).empty() && !KV.at(0).empty())
        {
            WAZUH_LOG_ERROR("Error while reading CDBfile");
            return false;
        }
        kvdb.write(KV.at(0), KV.at(1));
    }

    return true;
}

bool KVDBManager::deleteDB(const std::string &name)
{
    auto it = m_availableKVDBs.find(name);
    if (it == m_availableKVDBs.end())
    {
        WAZUH_LOG_ERROR("Database [{}] isn't handled by KVDB manager", name);
        return false;
    }
    bool ret = it->second->destroy();
    it->second.release();
    m_availableKVDBs.erase(it);
    return ret;
}

bool KVDBManager::addDB(const std::string &name,const std::string &folder)
{
    if (m_availableKVDBs.find(name) == m_availableKVDBs.end())
    {
        m_availableKVDBs[name] = std::make_unique<KVDB>(name, folder);
        return true;
    }
    return false;
}

KVDB &KVDBManager::getDB(const std::string &name)
{
    auto it = m_availableKVDBs.find(name);
    if (it != m_availableKVDBs.end())
    {
        return (*it->second);
    }
    return invalidDB;
}
