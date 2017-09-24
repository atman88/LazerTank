// persistChecker utility

#include <iostream>
#include <QFile>
#include <cstring>
#include <map>
#include "../persistfile.h"

#define INDEX_SIZE_TO_COUNT(size) ((int) ((size)-sizeof(PersistedLevelRecord)))

#define DANGLING 0x8000
#define MISMATCH  0x4000
#define ORPHANED 0x2000
#define LEVEL_FLAG_MASK (DANGLING|MISMATCH|ORPHANED)

int main( int argc, char *argv[] )
{
    const char* fileName = 0;
    bool clean = false;

    for( int i = 1; i < argc; ++i ) {
        char* arg = argv[i];
        if ( *arg == '-' ) {
            switch( arg[1] ) {
            case 'h':
                std::cerr << "use: " << argv[0] << " [-clean] file" << std::endl;
                exit(0);

            case 'c':
                if ( strncmp( arg, "clean", strlen(arg) ) ) {
                    clean = true;
                    continue;
                }
                break;
            default:
                ;
            }
            std::cerr << "Unknown option \"" << arg << "\". Use -h for usage" << std::endl;
            exit(1);
        } else {
            if ( fileName ) {
                std::cerr << "too many files specified" << std::endl;
            }
            fileName = arg;
        }
    }
    if ( !fileName ) {
        fileName = "/home/andy/qlt.sav";
    }

    QFile file( fileName );
    if ( !file.open( clean ? QIODevice::ReadWrite : QIODevice::ReadOnly ) ) {
        std::cerr << "couldn't open " << fileName << std::endl;
        exit(1);
    }

    std::map<qint64,PersistedLevelRecord> discoveredRecords;

    char RecordMagic[] = PERSISTED_LEVEL_RECORD_MAGIC_VALUE;
    while( !file.atEnd() ) {
        PersistedLevelRecord level;
        qint64 pos = file.pos();
        if ( file.read( level.magic, sizeof level.magic ) != sizeof level.magic ) {
            break;
        }
        if ( !std::memcmp( level.magic, RecordMagic, sizeof RecordMagic ) ) {
            // found a possible record
            if ( !file.seek( pos ) ) {
                std::cerr << "seek failed at " << pos << std::endl;
                exit(1);
            }

            if ( file.read( (char*) &level, sizeof level ) == sizeof level ) {
                if ( !(level.level & LEVEL_FLAG_MASK) ) {
                    discoveredRecords.insert( { pos, level } );
                } else {
                    std::cerr << "ignoring unsupported level #" << level.level << std::endl;
                }
            }
        }

        if ( !file.seek( pos+1 ) ) {
            std::cerr << "seek failed at " << pos << std::endl;
            exit(1);
        }
    }

    PersistedLevelIndexFooter indexFooter;
    auto fileSize = file.size();
    if ( fileSize < (int) sizeof(PersistedLevelIndexFooter) ) {
        std::cerr << "size " << fileSize << " too small to read index" << std::endl;
    } else if ( !file.seek( fileSize-sizeof(PersistedLevelIndexFooter) ) ) {
        std::cerr << "couldn't seek to index record" << std::endl;
    } else if ( file.read( (char*) &indexFooter, sizeof indexFooter ) != sizeof indexFooter ) {
        std::cerr << "couldn't read index footer" << std::endl;
    } else {
        char PersistentIndexFooterMagicValue[] = PERSISTED_INDEX_FOOTER_MAGIC_VALUE;
        if ( std::memcmp( indexFooter.magic, PersistentIndexFooterMagicValue, sizeof indexFooter.magic ) ) {
            std::cerr << "* index footer magic invalid: ";
            for( unsigned i = 0; i < sizeof indexFooter.magic; ++i ) {
                std::cerr << indexFooter.magic[i];
            }
            std::cerr << std::endl;
            exit(1);
        }
        if ( indexFooter.majorVersion != PersistedLevelIndexFooter::MajorVersionValue ) {
            std::cerr << "don't support index footer version " << indexFooter.majorVersion << "." << indexFooter.minorVersion << std::endl;
            exit(1);
        }
        if ( indexFooter.minorVersion != PersistedLevelIndexFooter::MinorVersionValue ) {
            std::cerr << "* index footer minor version: " << indexFooter.minorVersion << std::endl;
        }

        qint64 indexPos = fileSize - (sizeof indexFooter) - indexFooter.count * sizeof(PersistedLevelIndex);
        if ( !file.seek( indexPos ) ) {
            std::cerr << "couldn't seek to index list" << std::endl;
        } else {
            PersistedLevelIndex index;
            std::map<qint64,PersistedLevelIndex> indices;
            for( int i = 0; i < indexFooter.count; ++ i ) {
                if ( file.read( (char*) &index, sizeof index ) != sizeof index ) {
                    std::cerr << "couldn't read index #" << i << std::endl;
                    exit(1);
                }
                qint64 key = index.offset;
                auto checkIt = indices.find( key );
                if ( checkIt != indices.end() ) {
                    std::cerr << "* duplicate index for offset " << index.offset
                              << " {" << index.level << "," << index.size << "}"
                              << " {" << checkIt->second.level << "," << checkIt->second.size << "}" << std::endl;
                } else {
                    indices.insert( { key, index } );
                }
            }

            int cleanCount = 0;
            qint64 pos = 0;
            auto discoveredIt = discoveredRecords.begin();
            auto indexIt = indices.begin();
            while( indexIt != indices.end() ) {
                if ( pos > indexIt->first ) {
                    std::cout << "OVERLAP { " << indexIt->second.level << ", " << indexIt->second.size << " }" << std::endl;
                    indexIt->second.level |= ORPHANED;
                    ++indexIt;
                    continue;
                }
                if ( pos < indexIt->first ) {
                    std::cout << pos << ": " << (indexIt->first - pos ) << " bytes";
                    while( discoveredIt != discoveredRecords.end() && discoveredIt->first < indexIt->first ) {
                        std::cout << " " << discoveredIt->first << ":{" << discoveredIt->second.level << "," << discoveredIt->second.count << "}";
                        discoveredIt->second.level |= ORPHANED;
                        ++discoveredIt;
                    }
                    std::cout << std::endl;
                    pos = indexIt->first;
                }
                std::cout << pos << ": { " << indexIt->second.level << ", " << INDEX_SIZE_TO_COUNT(indexIt->second.size) << " }";
                if ( discoveredIt != discoveredRecords.end() ) {
                    if ( discoveredIt->first != indexIt->first ) {
                        std::cout << "-DANGLING";
                        indexIt->second.level |= ORPHANED;
                    } else if ( discoveredIt->second.level != indexIt->second.level
                             || discoveredIt->second.count != INDEX_SIZE_TO_COUNT(indexIt->second.size) ) {
                        discoveredIt->second.level |= MISMATCH;
                        indexIt->second.level |= MISMATCH;
                        std::cout << " MISMATCH {" << discoveredIt->second.level << "," << discoveredIt->second.count << "}";
                    } else {
                        ++discoveredIt;
                        ++cleanCount;
                    }
                }
                pos += indexIt->second.size;
                while( discoveredIt != discoveredRecords.end() && discoveredIt->first < pos ) {
                    std::cout << " " << discoveredIt->first << ":{" << discoveredIt->second.level << "," << discoveredIt->second.count << "}";
                    ++discoveredIt;
                }
                ++indexIt;
                std::cout << std::endl;
            }

            if ( pos < indexPos ) {
                std::cout << pos << ": " << (indexPos - pos ) << " bytes";
                while( discoveredIt != discoveredRecords.end() && discoveredIt->first < indexPos ) {
                    std::cout << " " << discoveredIt->first << ":{" << discoveredIt->second.level << "," << discoveredIt->second.count << "}";
                    discoveredIt->second.level |= ORPHANED;
                    ++discoveredIt;
                }
                std::cout << std::endl;
                pos = indexPos;
            }

            if ( clean ) {
                if ( cleanCount < indexFooter.count ) {
                    if ( !file.seek( file.size() - sizeof(indexFooter) - cleanCount * sizeof(PersistedLevelIndex) ) ) {
                        std::cerr << "couldn't seek to index" << std::endl;
                        exit(1);
                    }

                    indexIt = indices.begin();
                    int i = 0;
                    while( i < cleanCount && indexIt != indices.end() ) {
                        if ( !(indexIt->second.level & LEVEL_FLAG_MASK) ) {
                            if ( file.write( (char*) &indexIt->second, sizeof(PersistedLevelIndex) ) != sizeof(PersistedLevelIndex) ) {
                                std::cerr << "write index failed" << std::endl;
                                exit(1);
                            }
                            ++i;
                        }
                        ++indexIt;
                    }
                    indexFooter.count = i;
                    if ( file.write( (char*) &indexFooter, sizeof indexFooter ) != sizeof indexFooter ) {
                        std::cerr << "write index footer failed" << std::endl;
                        exit(1);
                    }
                    std::cout << "Removed " << (indices.size() - i) << " records from the index" << std::endl;
                }
            }
        }
    }
}
