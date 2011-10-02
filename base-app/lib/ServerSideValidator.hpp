/** Provides a server side validator.
  * Copyright Matthew D.G. Sherborne.
  * License GPL2 .. see LICENSE file.
  */
#ifndef SERVERSIDEVALIDATOR_HPP
#define SERVERSIDEVALIDATOR_HPP

#include <stdexcept>
#include <string>
#include <sstream>
#include <Wt/WApplication>
#include <Wt/WString>
#include <Wt/WValidator>
#include <Wt/WSignal>
#include <Wt/WFormWidget>
#include <Wt/WAbstractToggleButton>
#include <Wt/WComboBox>
#include <Wt/WLineEdit>
#include <Wt/WPushButton>
#include <Wt/WSlider>
#include <Wt/WTextArea>
#include <stdexcept>

using Wt::WValidator;
using Wt::Signal;
using Wt::WString;
using Wt::WFormWidget;

namespace wittyPlus {

/// Holds a validation result, plus a message for the user
struct ServerSideValidationResult {
    ServerSideValidationResult(const WString& message, WValidator::State result) : message(message), result(result) {}
    WString message;
    WValidator::State result;
};

/** The standard Wt::WValidator seems to make the assumption that nearly all validation that requires showing a message
  * will be done on the client side.
  * This class extends Wt::WValidator to provide a 'validateWithMessage' function on the server side.
  * It is intended to be used with validation that needs to be done server side, but where a message must be shown
  * to the client also. We don't show the message though, we just provide it in a signal when validate is called
  **/
class ServerSideValidator : public WValidator {
public:
    typedef Signal<State, WString> MsgSignal;
protected:
    WString _validMsg;
    WString _invalidMsg;
public:
    ServerSideValidator(
        const WString& validMsg="", const WString& invalidMsg="", const WString& emptyMsg="",
        bool isMandatory=false, WObject* parent=0)
        : WValidator(isMandatory, parent), _validMsg(validMsg), _invalidMsg(invalidMsg)
    {
        if (!emptyMsg.empty())
            setInvalidBlankText(emptyMsg);
    }
    ServerSideValidator(bool isMandatory, WObject* parent=0)
        : WValidator(isMandatory, parent), _validMsg(""), _invalidMsg("")
    {}
    /// Call this instead of validate when you want to get the message on the server side
    virtual ServerSideValidationResult validateWithMessage(WString& value) const {
        State validness = validate(value);
        return ServerSideValidationResult(state2msg(validness), validness);
    }
    /// Turns the output of Wt::WValidator::validate into a message for the end user
    WString state2msg(State validationResult) const {
        switch (validationResult) {
            case Valid: return _validMsg;
            case Invalid: return _invalidMsg;
            case InvalidEmpty: return invalidBlankText();
            default:
                throw std::logic_error("Did they add a new value to Wt::WValidator::State or something ?");
        }
    }
    // Accessors
    const WString& getValidMsg() { return _validMsg; }
    void setValidMsg(const WString& newMsg) { _validMsg = newMsg; }
    const WString& getInvalidMsg() { return _invalidMsg; }
    void setInvalidMsg(const WString& newMsg) { _invalidMsg = newMsg; }
    // Static Functions
    /// Returns the value of pretty much all widget types
    static WString getValue(WFormWidget* widget) {
        Wt::WComboBox* asComboBox = dynamic_cast<Wt::WComboBox*>(widget);
        if (asComboBox != 0)
            return asComboBox->currentText();
        Wt::WLineEdit* asLineEdit = dynamic_cast<Wt::WLineEdit*>(widget);
        if (asLineEdit != 0)
            return asLineEdit->text();
        Wt::WSlider* asSlider = dynamic_cast<Wt::WSlider*>(widget);
        if (asSlider != 0) {
            std::stringstream out;
            out << asSlider->value();
            return out.str();
        }
        Wt::WTextArea* asTextArea = dynamic_cast<Wt::WTextArea*>(widget);
        if (asTextArea != 0)
            return asTextArea->text();
        throw std::logic_error("I don't know how to get the value of whatever widget you passed me");
    }
    /// Call this to push a server side validation result to the client side (complete with message)
    static void tellClient(WFormWidget* widget, const WString& value, ServerSideValidationResult validationResult) {
        widget->setJavaScriptMember("serverValidationResult",
         "{ value:" + value.jsStringLiteral() + ","
          "valid:" + (validationResult.result == WValidator::Valid ? "true," : "false,") +
         "message:" + validationResult.message.jsStringLiteral() + "}");
        Wt::WApplication* app = Wt::WApplication::instance();
        app->doJavaScript( app->javaScriptClass() + ".WT.validate(" + widget->jsRef() + ");" );
    }
    /// Convenience function. Call this instead of widget.validate();
    static ServerSideValidationResult validateWidget(WFormWidget* widget) {
        ServerSideValidator* validator = dynamic_cast<ServerSideValidator*>(widget->validator());
        if (validator != 0) {
            WString value = getValue(widget);
            return validator->validateWithMessage(value);
        } else {
            return ServerSideValidationResult("", widget->validate());
        }
    }
    /// Validates the widget and passes the message on to the browser
    static ServerSideValidationResult validateWidgetAndTellBrowser(WFormWidget* widget) {
        ServerSideValidator* validator = dynamic_cast<ServerSideValidator*>(widget->validator());
        if (validator != 0) {
            WString value = getValue(widget);
            ServerSideValidationResult result = validator->validateWithMessage(value);
            tellClient(widget, value, result);
            return result;
        } else {
            Wt::WApplication* app = Wt::WApplication::instance();
            app->doJavaScript( app->javaScriptClass() + ".WT.validate(" + widget->jsRef() + ");" );
            return ServerSideValidationResult("", widget->validate());
        }
    }

};

} // namespace wittyPlus
#endif // SERVERSIDEVALIDATOR_HPP
