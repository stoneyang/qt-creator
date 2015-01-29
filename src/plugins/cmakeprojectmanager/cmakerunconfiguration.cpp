/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cmakerunconfiguration.h"

#include "cmakebuildconfiguration.h"
#include "cmakeproject.h"
#include "cmakeprojectconstants.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/helpmanager.h>
#include <qtsupport/qtkitinformation.h>
#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/target.h>

#include <utils/pathchooser.h>
#include <utils/detailswidget.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>

#include <QFormLayout>
#include <QLineEdit>
#include <QGroupBox>
#include <QLabel>
#include <QComboBox>
#include <QToolButton>
#include <QCheckBox>

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;
using namespace ProjectExplorer;

namespace {
const char CMAKE_RC_PREFIX[] = "CMakeProjectManager.CMakeRunConfiguration.";

const char USER_WORKING_DIRECTORY_KEY[] = "CMakeProjectManager.CMakeRunConfiguration.UserWorkingDirectory";
const char USE_TERMINAL_KEY[] = "CMakeProjectManager.CMakeRunConfiguration.UseTerminal";
const char TITLE_KEY[] = "CMakeProjectManager.CMakeRunConfiguation.Title";
const char ARGUMENTS_KEY[] = "CMakeProjectManager.CMakeRunConfiguration.Arguments";

} // namespace

CMakeRunConfiguration::CMakeRunConfiguration(Target *parent, Core::Id id, const QString &target,
                                             const QString &workingDirectory, const QString &title) :
    LocalApplicationRunConfiguration(parent, id),
    m_runMode(ProjectExplorer::ApplicationLauncher::Gui),
    m_buildTarget(target),
    m_workingDirectory(workingDirectory),
    m_title(title),
    m_enabled(true)
{
    addExtraAspect(new LocalEnvironmentAspect(this));
    ctor();
}

CMakeRunConfiguration::CMakeRunConfiguration(Target *parent, CMakeRunConfiguration *source) :
    LocalApplicationRunConfiguration(parent, source),
    m_runMode(source->m_runMode),
    m_buildTarget(source->m_buildTarget),
    m_workingDirectory(source->m_workingDirectory),
    m_userWorkingDirectory(source->m_userWorkingDirectory),
    m_title(source->m_title),
    m_arguments(source->m_arguments),
    m_enabled(source->m_enabled)
{
    ctor();
}

CMakeRunConfiguration::~CMakeRunConfiguration()
{
}

void CMakeRunConfiguration::ctor()
{
    setDefaultDisplayName(defaultDisplayName());
}

QString CMakeRunConfiguration::executable() const
{
    return m_buildTarget;
}

ApplicationLauncher::Mode CMakeRunConfiguration::runMode() const
{
    return m_runMode;
}

void CMakeRunConfiguration::setRunMode(ApplicationLauncher::Mode runMode)
{
    m_runMode = runMode;
}

QString CMakeRunConfiguration::workingDirectory() const
{
    EnvironmentAspect *aspect = extraAspect<EnvironmentAspect>();
    QTC_ASSERT(aspect, return QString());
    return QDir::cleanPath(aspect->environment().expandVariables(
                macroExpander()->expand(baseWorkingDirectory())));
}

QString CMakeRunConfiguration::baseWorkingDirectory() const
{
    if (!m_userWorkingDirectory.isEmpty())
        return m_userWorkingDirectory;
    return m_workingDirectory;
}

QString CMakeRunConfiguration::commandLineArguments() const
{
    return macroExpander()->expandProcessArgs(m_arguments);
}

QString CMakeRunConfiguration::title() const
{
    return m_title;
}

void CMakeRunConfiguration::setExecutable(const QString &executable)
{
    m_buildTarget = executable;
}

void CMakeRunConfiguration::setBaseWorkingDirectory(const QString &wd)
{
    const QString &oldWorkingDirectory = workingDirectory();

    m_workingDirectory = wd;

    const QString &newWorkingDirectory = workingDirectory();
    if (oldWorkingDirectory != newWorkingDirectory)
        emit baseWorkingDirectoryChanged(newWorkingDirectory);
}

void CMakeRunConfiguration::setUserWorkingDirectory(const QString &wd)
{
    const QString & oldWorkingDirectory = workingDirectory();

    m_userWorkingDirectory = wd;

    const QString &newWorkingDirectory = workingDirectory();
    if (oldWorkingDirectory != newWorkingDirectory)
        emit baseWorkingDirectoryChanged(newWorkingDirectory);
}

QVariantMap CMakeRunConfiguration::toMap() const
{
    QVariantMap map(LocalApplicationRunConfiguration::toMap());

    map.insert(QLatin1String(USER_WORKING_DIRECTORY_KEY), m_userWorkingDirectory);
    map.insert(QLatin1String(USE_TERMINAL_KEY), m_runMode == ApplicationLauncher::Console);
    map.insert(QLatin1String(TITLE_KEY), m_title);
    map.insert(QLatin1String(ARGUMENTS_KEY), m_arguments);

    return map;
}

bool CMakeRunConfiguration::fromMap(const QVariantMap &map)
{
    m_userWorkingDirectory = map.value(QLatin1String(USER_WORKING_DIRECTORY_KEY)).toString();
    m_runMode = map.value(QLatin1String(USE_TERMINAL_KEY)).toBool() ? ApplicationLauncher::Console
                                                                    : ApplicationLauncher::Gui;
    m_title = map.value(QLatin1String(TITLE_KEY)).toString();
    m_arguments = map.value(QLatin1String(ARGUMENTS_KEY)).toString();

    return RunConfiguration::fromMap(map);
}

QString CMakeRunConfiguration::defaultDisplayName() const
{
    if (m_title.isEmpty())
        return tr("Run CMake kit");
    QString result = m_title;
    if (!m_enabled) {
        result += QLatin1Char(' ');
        result += tr("(disabled)");
    }
    return result;
}

QWidget *CMakeRunConfiguration::createConfigurationWidget()
{
    return new CMakeRunConfigurationWidget(this);
}

void CMakeRunConfiguration::setCommandLineArguments(const QString &newText)
{
    m_arguments = newText;
}

void CMakeRunConfiguration::setEnabled(bool b)
{
    if (m_enabled == b)
        return;
    m_enabled = b;
    emit enabledChanged();
    setDefaultDisplayName(defaultDisplayName());
}

bool CMakeRunConfiguration::isEnabled() const
{
    return m_enabled;
}

QString CMakeRunConfiguration::disabledReason() const
{
    if (!m_enabled)
        return tr("The executable is not built by the current build configuration");
    return QString();
}

// Configuration widget
CMakeRunConfigurationWidget::CMakeRunConfigurationWidget(CMakeRunConfiguration *cmakeRunConfiguration, QWidget *parent)
    : QWidget(parent), m_ignoreChange(false), m_cmakeRunConfiguration(cmakeRunConfiguration)
{
    QFormLayout *fl = new QFormLayout();
    fl->setMargin(0);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    QLineEdit *argumentsLineEdit = new QLineEdit();
    argumentsLineEdit->setText(cmakeRunConfiguration->commandLineArguments());
    connect(argumentsLineEdit, &QLineEdit::textChanged, this, &CMakeRunConfigurationWidget::setArguments);
    fl->addRow(tr("Arguments:"), argumentsLineEdit);

    m_workingDirectoryEdit = new Utils::PathChooser();
    m_workingDirectoryEdit->setExpectedKind(Utils::PathChooser::Directory);
    m_workingDirectoryEdit->setBaseFileName(m_cmakeRunConfiguration->target()->project()->projectDirectory());
    m_workingDirectoryEdit->setPath(m_cmakeRunConfiguration->baseWorkingDirectory());
    m_workingDirectoryEdit->setHistoryCompleter(QLatin1String("CMake.WorkingDir.History"));
    EnvironmentAspect *aspect
            = m_cmakeRunConfiguration->extraAspect<EnvironmentAspect>();
    if (aspect) {
        connect(aspect, &EnvironmentAspect::environmentChanged,
                this, &CMakeRunConfigurationWidget::environmentWasChanged);
        environmentWasChanged();
    }
    m_workingDirectoryEdit->setPromptDialogTitle(tr("Select Working Directory"));

    QToolButton *resetButton = new QToolButton();
    resetButton->setToolTip(tr("Reset to default."));
    resetButton->setIcon(QIcon(QLatin1String(Core::Constants::ICON_RESET)));

    QHBoxLayout *boxlayout = new QHBoxLayout();
    boxlayout->addWidget(m_workingDirectoryEdit);
    boxlayout->addWidget(resetButton);

    fl->addRow(tr("Working directory:"), boxlayout);

    QCheckBox *runInTerminal = new QCheckBox;
    fl->addRow(tr("Run in Terminal"), runInTerminal);

    m_detailsContainer = new Utils::DetailsWidget(this);
    m_detailsContainer->setState(Utils::DetailsWidget::NoSummary);

    QWidget *m_details = new QWidget(m_detailsContainer);
    m_detailsContainer->setWidget(m_details);
    m_details->setLayout(fl);

    QVBoxLayout *vbx = new QVBoxLayout(this);
    vbx->setMargin(0);
    vbx->addWidget(m_detailsContainer);

    connect(m_workingDirectoryEdit, &Utils::PathChooser::changed,
            this, &CMakeRunConfigurationWidget::setWorkingDirectory);

    connect(resetButton, &QToolButton::clicked,
            this, &CMakeRunConfigurationWidget::resetWorkingDirectory);

    connect(runInTerminal, &QCheckBox::toggled,
            this, &CMakeRunConfigurationWidget::runInTerminalToggled);

    connect(m_cmakeRunConfiguration, &CMakeRunConfiguration::baseWorkingDirectoryChanged,
            this, &CMakeRunConfigurationWidget::workingDirectoryChanged);

    setEnabled(m_cmakeRunConfiguration->isEnabled());
}

void CMakeRunConfigurationWidget::setWorkingDirectory()
{
    if (m_ignoreChange)
        return;
    m_ignoreChange = true;
    m_cmakeRunConfiguration->setUserWorkingDirectory(m_workingDirectoryEdit->rawPath());
    m_ignoreChange = false;
}

void CMakeRunConfigurationWidget::workingDirectoryChanged(const QString &workingDirectory)
{
    if (!m_ignoreChange) {
        m_ignoreChange = true;
        m_workingDirectoryEdit->setPath(workingDirectory);
        m_ignoreChange = false;
    }
}

void CMakeRunConfigurationWidget::resetWorkingDirectory()
{
    // This emits a signal connected to workingDirectoryChanged()
    // that sets the m_workingDirectoryEdit
    m_cmakeRunConfiguration->setUserWorkingDirectory(QString());
}

void CMakeRunConfigurationWidget::runInTerminalToggled(bool toggled)
{
    m_cmakeRunConfiguration->setRunMode(toggled ? ApplicationLauncher::Console
                                                : ApplicationLauncher::Gui);
}

void CMakeRunConfigurationWidget::environmentWasChanged()
{
    EnvironmentAspect *aspect = m_cmakeRunConfiguration->extraAspect<EnvironmentAspect>();
    QTC_ASSERT(aspect, return);
    m_workingDirectoryEdit->setEnvironment(aspect->environment());
}
void CMakeRunConfigurationWidget::setArguments(const QString &args)
{
    m_cmakeRunConfiguration->setCommandLineArguments(args);
}

// Factory
CMakeRunConfigurationFactory::CMakeRunConfigurationFactory(QObject *parent) :
    IRunConfigurationFactory(parent)
{ setObjectName(QLatin1String("CMakeRunConfigurationFactory")); }

CMakeRunConfigurationFactory::~CMakeRunConfigurationFactory()
{
}

// used to show the list of possible additons to a project, returns a list of ids
QList<Core::Id> CMakeRunConfigurationFactory::availableCreationIds(Target *parent, CreationMode mode) const
{
    Q_UNUSED(mode)
    if (!canHandle(parent))
        return QList<Core::Id>();
    CMakeProject *project = static_cast<CMakeProject *>(parent->project());
    QList<Core::Id> allIds;
    foreach (const QString &buildTarget, project->buildTargetTitles(true))
        allIds << idFromBuildTarget(buildTarget);
    return allIds;
}

// used to translate the ids to names to display to the user
QString CMakeRunConfigurationFactory::displayNameForId(Core::Id id) const
{
    return buildTargetFromId(id);
}

bool CMakeRunConfigurationFactory::canHandle(Target *parent) const
{
    if (!parent->project()->supportsKit(parent->kit()))
        return false;
    return qobject_cast<CMakeProject *>(parent->project());
}

bool CMakeRunConfigurationFactory::canCreate(Target *parent, Core::Id id) const
{
    if (!canHandle(parent))
        return false;
    CMakeProject *project = static_cast<CMakeProject *>(parent->project());
    return project->hasBuildTarget(buildTargetFromId(id));
}

RunConfiguration *CMakeRunConfigurationFactory::doCreate(Target *parent, Core::Id id)
{
    CMakeProject *project = static_cast<CMakeProject *>(parent->project());
    const QString title(buildTargetFromId(id));
    const CMakeBuildTarget &ct = project->buildTargetForTitle(title);
    return new CMakeRunConfiguration(parent, id, ct.executable, ct.workingDirectory, ct.title);
}

bool CMakeRunConfigurationFactory::canClone(Target *parent, RunConfiguration *source) const
{
    if (!canHandle(parent))
        return false;
    return source->id().name().startsWith(CMAKE_RC_PREFIX);
}

RunConfiguration *CMakeRunConfigurationFactory::clone(Target *parent, RunConfiguration * source)
{
    if (!canClone(parent, source))
        return 0;
    CMakeRunConfiguration *crc(static_cast<CMakeRunConfiguration *>(source));
    return new CMakeRunConfiguration(parent, crc);
}

bool CMakeRunConfigurationFactory::canRestore(Target *parent, const QVariantMap &map) const
{
    if (!qobject_cast<CMakeProject *>(parent->project()))
        return false;
    return idFromMap(map).name().startsWith(CMAKE_RC_PREFIX);
}

RunConfiguration *CMakeRunConfigurationFactory::doRestore(Target *parent, const QVariantMap &map)
{
    return new CMakeRunConfiguration(parent, idFromMap(map), QString(), QString(), QString());
}

QString CMakeRunConfigurationFactory::buildTargetFromId(Core::Id id)
{
    return id.suffixAfter(CMAKE_RC_PREFIX);
}

Core::Id CMakeRunConfigurationFactory::idFromBuildTarget(const QString &target)
{
    return Core::Id(CMAKE_RC_PREFIX).withSuffix(target);
}
