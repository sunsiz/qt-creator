#include "qmljsast_p.h"
#include "qmljsengine_p.h"
#include "qmlresolveexpression.h"

using namespace DuiEditor;
using namespace DuiEditor::Internal;
using namespace QmlJS;
using namespace QmlJS::AST;

QmlResolveExpression::QmlResolveExpression(const QmlLookupContext &context)
    : _context(context), _value(0)
{
}

QmlResolveExpression::~QmlResolveExpression()
{
    qDeleteAll(_temporarySymbols);
}

QmlSymbol *QmlResolveExpression::typeOf(Node *node)
{
    QmlSymbol *previousValue = switchValue(0);
    if (node)
        node->accept(this);
    return switchValue(previousValue);
}


QmlSymbol *QmlResolveExpression::switchValue(QmlSymbol *value)
{
    QmlSymbol *previousValue = _value;
    _value = value;
    return previousValue;
}

bool QmlResolveExpression::visit(IdentifierExpression *ast)
{
    const QString name = ast->name->asString();
    _value = _context.resolve(name);
    return false;
}

static inline bool matches(UiQualifiedId *candidate, const QString &wanted)
{
    if (!candidate)
        return false;

    if (!(candidate->name))
        return false;

    if (candidate->next)
        return false; // TODO: verify this!

    return wanted == candidate->name->asString();
}

bool QmlResolveExpression::visit(FieldMemberExpression *ast)
{
    const QString memberName = ast->name->asString();

    const QmlSymbol *base = typeOf(ast->base);
    if (!base)
        return false;

    if (base->isIdSymbol())
        base = base->asIdSymbol()->parentNode();

    UiObjectMemberList *members = 0;

    if (const QmlSymbolFromFile *symbol = base->asSymbolFromFile()) {
        Node *node = symbol->node();

        if (UiObjectBinding *binding = cast<UiObjectBinding*>(node)) {
            if (binding->initializer)
                members = binding->initializer->members;
        } else if (UiObjectDefinition *definition = cast<UiObjectDefinition*>(node)) {
            if (definition->initializer)
                members = definition->initializer->members;
        }
    }

    for (UiObjectMemberList *it = members; it; it = it->next) {
        UiObjectMember *member = it->member;

        if (UiPublicMember *publicMember = cast<UiPublicMember *>(member)) {
            if (publicMember->name && publicMember->name->asString() == memberName) {
                _value = createPropertyDefinitionSymbol(publicMember);
                break; // we're done.
            }
        } else if (UiObjectBinding *objectBinding = cast<UiObjectBinding*>(member)) {
            if (matches(objectBinding->qualifiedId, memberName)) {
                _value = createSymbolFromFile(objectBinding);
                break; // we're done
            }
        } else if (UiScriptBinding *scriptBinding = cast<UiScriptBinding*>(member)) {
            if (matches(scriptBinding->qualifiedId, memberName)) {
                _value = createSymbolFromFile(scriptBinding);
                break; // we're done
            }
        } else if (UiArrayBinding *arrayBinding = cast<UiArrayBinding*>(member)) {
            if (matches(arrayBinding->qualifiedId, memberName)) {
                _value = createSymbolFromFile(arrayBinding);
                break; // we're done
            }
        }
    }

    return false;
}

bool QmlResolveExpression::visit(QmlJS::AST::UiQualifiedId *ast)
{
    _value = _context.resolveType(ast);

    return false;
}

QmlPropertyDefinitionSymbol *QmlResolveExpression::createPropertyDefinitionSymbol(QmlJS::AST::UiPublicMember *ast)
{
    QmlPropertyDefinitionSymbol *symbol = new QmlPropertyDefinitionSymbol(_context.document()->fileName(), ast);
    _temporarySymbols.append(symbol);
    return symbol;
}

QmlSymbolFromFile *QmlResolveExpression::createSymbolFromFile(QmlJS::AST::UiObjectMember *ast)
{
    QmlSymbolFromFile *symbol = new QmlSymbolFromFile(_context.document()->fileName(), ast);
    _temporarySymbols.append(symbol);
    return symbol;
}
