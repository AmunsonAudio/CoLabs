// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell


#pragma once

#include "JuceHeader.h"

#include "SonoTextButton.h"
#include "GenericItemChooser.h"
#include "SonoLookAndFeel.h"

class SonoChoiceLookAndFeel : public SonoLookAndFeel
{
public:
    int getCallOutBoxBorderSize (const CallOutBox&) override {
        return 40;
    }
    
};



class SonoChoiceButton : public SonoTextButton, public GenericItemChooser::Listener, public Button::Listener
{
public:
    SonoChoiceButton();
    virtual ~SonoChoiceButton();
    
    class Listener {
    public:
        virtual ~Listener() {}
        virtual void choiceButtonSelected(SonoChoiceButton *comp, int index, int ident) {}
    };

    void paint(Graphics & g) override;
    void resized() override;

    void genericItemChooserSelected(GenericItemChooser *comp, int index) override;
    
    void clearItems();
    void addItem(const String & name, int ident, bool separator=false, bool disabled=false);
    void addItem(const String & name, int ident, const Image & newItemImage, bool separator=false, bool disabled=false);
    
    //void setItems(const StringArray & items);
    //const StringArray & getItems() const { return items; }

    void setSelectedItemIndex(int index, NotificationType notification = dontSendNotification);
    int getSelectedItemIndex() const { return selIndex; }

    void setSelectedId(int ident, NotificationType notification = dontSendNotification);

    void setShowArrow(bool flag) { showArrow = flag; }
    bool getShowArrow() const { return showArrow; }
    
    String getItemText(int index) const;

    String getCurrentText() const;

    void buttonClicked (Button* buttonThatWasClicked) override;

    void addChoiceListener(Listener * listener) { listeners.add(listener); }
    void removeChoiceListener(Listener * listener) { listeners.remove(listener); }

    void showPopup();
    bool isPopupActive() const;

    std::unique_ptr<AccessibilityHandler> createAccessibilityHandler() override;

protected:
    ListenerList<Listener> listeners;

    std::unique_ptr<Label> textLabel;

    Array<GenericItemChooserItem> items;
    Array<int> idList;
    
    int selIndex;
    bool showArrow = true;

    WeakReference<Component> activeCalloutBox;

    SonoChoiceLookAndFeel lnf;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SonoChoiceButton)

};


