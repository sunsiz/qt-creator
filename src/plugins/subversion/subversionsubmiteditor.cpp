/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "subversionsubmiteditor.h"

#include <coreplugin/idocument.h>
#include <vcsbase/submiteditorwidget.h>
#include <vcsbase/submitfilemodel.h>

using namespace Subversion::Internal;

SubversionSubmitEditor::SubversionSubmitEditor(const VcsBase::VcsBaseSubmitEditorParameters *parameters) :
    VcsBase::VcsBaseSubmitEditor(parameters, new VcsBase::SubmitEditorWidget)
{
    document()->setPreferredDisplayName(tr("Subversion Submit"));
    setDescriptionMandatory(false);
}

void SubversionSubmitEditor::setStatusList(const QList<StatusFilePair> &statusOutput)
{
    typedef QList<StatusFilePair>::const_iterator ConstIterator;
    auto model = new VcsBase::SubmitFileModel(this);
    // Hack to allow completion in "description" field : completion needs a root repository, the
    // checkScriptWorkingDirectory property is fine (at this point it was set by SubversionPlugin)
    model->setRepositoryRoot(checkScriptWorkingDirectory());
    model->setFileStatusQualifier([](const QString &status, const QVariant &)
                                  -> VcsBase::SubmitFileModel::FileStatusHint
    {
        if (status == QLatin1String("C"))
            return VcsBase::SubmitFileModel::FileUnmerged;
        if (status == QLatin1String("A"))
            return VcsBase::SubmitFileModel::FileAdded;
        if (status == QLatin1String("M"))
            return VcsBase::SubmitFileModel::FileModified;
        if (status == QLatin1String("D"))
            return VcsBase::SubmitFileModel::FileDeleted;
        return VcsBase::SubmitFileModel::FileStatusUnknown;
    } );

    const ConstIterator cend = statusOutput.constEnd();
    for (ConstIterator it = statusOutput.constBegin(); it != cend; ++it)
        model->addFile(it->second, it->first);
    setFileModel(model);
}

QByteArray SubversionSubmitEditor::fileContents() const
{
    return description().toUtf8();
}

bool SubversionSubmitEditor::setFileContents(const QByteArray &contents)
{
    setDescription(QString::fromUtf8(contents));
    return true;
}
