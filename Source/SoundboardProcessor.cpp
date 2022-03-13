// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell



#include "SoundboardProcessor.h"
#include "SonoUtility.h"
#include <utility>

SoundboardProcessor::SoundboardProcessor(SoundboardChannelProcessor* channelProcessor, File supportDir) : channelProcessor(
        channelProcessor)
{
    soundboardsFile = supportDir.getChildFile("soundboards.xml");

    loadFromDisk();
}

Soundboard& SoundboardProcessor::addSoundboard(const String& name, const bool select)
{
    auto newSoundboard = Soundboard(name);
    soundboards.push_back(std::move(newSoundboard));

    if (select) {
        selectedSoundboardIndex = getNumberOfSoundboards() - 1;
    }

    reorderSoundboards();
    saveToDisk();

    return soundboards[selectedSoundboardIndex.value_or(0)];
}

void SoundboardProcessor::renameSoundboard(int index, String newName)
{
    auto& toRename = soundboards[index];
    toRename.setName(std::move(newName));

    reorderSoundboards();
    saveToDisk();
}

void SoundboardProcessor::deleteSoundboard(int index)
{
    // When a sample from this soundboard was playing, stop playback
    auto& activeSamples = channelProcessor->getActiveSamples();
    for (const auto& sample : soundboards[index].getSamples()) {
        auto playbackManager = activeSamples.find(&sample);
        if (playbackManager != activeSamples.end()) {
            playbackManager->second->unload();
        }
    }

    soundboards.erase(soundboards.begin() + index);

    // If the last soundboard was selected
    if (selectedSoundboardIndex == soundboards.size()) {
        auto selected = *selectedSoundboardIndex;
        selectedSoundboardIndex = selected > 0 ? std::optional<int>(selected - 1) : std::nullopt;
    }

    reorderSoundboards();
    saveToDisk();
}

void SoundboardProcessor::selectSoundboard(int index)
{
    if (getNumberOfSoundboards() == 0) {
        selectedSoundboardIndex = {};
    }
    else {
        selectedSoundboardIndex = jmax(0, jmin(index, static_cast<int>(getNumberOfSoundboards())));
    }

    saveToDisk();
}

void SoundboardProcessor::reorderSoundboards()
{
    // Figure out what the new (sorted) indices will be.
    auto originalSelectedIndex = selectedSoundboardIndex.value_or(-1);
    auto originalIndices = sortIndexPreview(soundboards);

    // Determine new indices of the selected soundboard.
    if (originalSelectedIndex < 0) {
        selectedSoundboardIndex = { 0 };
    }
    else {
        auto iterator = std::find(originalIndices.begin(), originalIndices.end(), originalSelectedIndex);
        selectedSoundboardIndex = { std::distance(originalIndices.begin(), iterator) };
    }

    // Above was just a sort preview and logic on that. End with actually sorting the list of soundboards.
    std::sort(soundboards.begin(), soundboards.end(), [](const Soundboard& a, const Soundboard& b) {
        return a.getName() < b.getName();
    });
}

SoundSample* SoundboardProcessor::addSoundSample(String name, String absolutePath, std::optional<int> index)
{
    // Per definition: do nothing when no soundboard is selected or specified.
    if (!index.has_value() && !selectedSoundboardIndex.has_value()) {
        return nullptr;
    }

    auto sindex = index.has_value() ? *index : *selectedSoundboardIndex;
    if (sindex < 0 || sindex >= soundboards.size())
        return nullptr;

    auto& soundboard = soundboards[sindex];
    auto& sampleList = soundboard.getSamples();

    SoundSample sampleToAdd = SoundSample(std::move(name), std::move(absolutePath));
    sampleList.emplace_back(std::move(sampleToAdd));

    saveToDisk();

    return &sampleList[sampleList.size() - 1];
}

void SoundboardProcessor::editSoundSample(SoundSample& sampleToUpdate)
{
    saveToDisk();

    // Immediately update transport source with new playback settings when this sample is currently playing
    auto& activeSamples = channelProcessor->getActiveSamples();
    auto playbackManager = activeSamples.find(&sampleToUpdate);
    if (playbackManager != activeSamples.end()) {
        playbackManager->second->reloadPlaybackSettingsFromSample();
    }
}

bool SoundboardProcessor::deleteSoundSample(SoundSample& sampleToDelete, std::optional<int> index)
{
    if (!index.has_value() && !selectedSoundboardIndex.has_value()) {
        return false;
    }

    auto sindex = index.has_value() ? *index : *selectedSoundboardIndex;
    if (sindex < 0 || sindex >= soundboards.size())
        return false;

    auto& soundboard = soundboards[sindex];
    auto& sampleList = soundboard.getSamples();

    for (auto iter = sampleList.begin(); iter != sampleList.end(); ++iter) {
        auto& sample = *iter;
        if (&sample == &sampleToDelete) {
            // If it is currently playing, unload
            auto& activeSamples = channelProcessor->getActiveSamples();
            auto playbackManager = activeSamples.find(&sample);
            if (playbackManager != activeSamples.end()) {
                playbackManager->second->unload();
            }

            sampleList.erase(iter);

            break;
        }
    }

    saveToDisk();
    return true;
}

void SoundboardProcessor::stopAllPlayback()
{
    channelProcessor->unloadAll();
}

void SoundboardProcessor::writeSoundboardsToFile(const File& file) const
{
    ValueTree tree(SOUNDBOARDS_KEY);

    tree.setProperty(SELECTED_KEY, selectedSoundboardIndex.value_or(-1), nullptr);
    tree.setProperty(HOTKEYS_MUTED_KEY, hotkeysMuted, nullptr);

    int i = 0;
    for (const auto& soundboard: soundboards) {
        tree.addChild(soundboard.serialize(), i++, nullptr);
    }

    // Make sure  the parent directory exists
    file.getParentDirectory().createDirectory();

    tree.createXml()->writeTo(file);
}

void SoundboardProcessor::readSoundboardsFromFile(const File& file)
{
    if (!file.existsAsFile()) {
        return;
    }

    XmlDocument doc(file);
    auto tree = ValueTree::fromXml(*doc.getDocumentElement());

    int selected = tree.getProperty(SELECTED_KEY);
    selectedSoundboardIndex = selected >= 0 ? std::optional<size_t>(selected) : std::nullopt;
    hotkeysMuted = tree.getProperty(HOTKEYS_MUTED_KEY, false);

    soundboards.clear();

    for (const auto& child: tree) {
        soundboards.emplace_back(Soundboard::deserialize(child));
    }
}

void SoundboardProcessor::saveToDisk() const
{
    writeSoundboardsToFile(soundboardsFile);
}

void SoundboardProcessor::loadFromDisk()
{
    readSoundboardsFromFile(soundboardsFile);
    reorderSoundboards();
}
