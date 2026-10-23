#pragma once
#include <hustmcl/application/modpack_import.h>
#include <QByteArray>
#include <QString>
#include <atomic>

namespace hustmcl::services::pack {
struct ArchiveEntry { unsigned index; QString path; quint64 size; bool directory; };
struct Override { unsigned index; QString path; quint64 size; int layer; };
struct ArchivePack { core::Modpack pack; QString archive, base; std::vector<Override> overrides; };
// Throws a typed ImportFailure; never extracts a path supplied by the archive directly.
std::vector<ArchiveEntry> archiveEntries(const QString& archive);
QByteArray archiveBytes(const QString& archive, unsigned index, quint64 maximum);
void extractEntry(const QString& archive, unsigned index, const QString& destination, const std::atomic_bool& cancel);
ArchivePack inspectArchive(const QString& archive, const QString& innerFile, const std::atomic_bool& cancel);
}
