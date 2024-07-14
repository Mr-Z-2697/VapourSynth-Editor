#include "../../common-src/settings/settings_manager.h"
#include "../../common-src/vapoursynth/vs_script_library.h"
#include "../../common-src/log/vs_editor_log_definitions.h"
#include "../../common-src/helpers.h"

#include "frame_consumers/encode_dialog.h"

#include <QFileInfo>
#include <QMessageBox>
#include <QResource>

#include <iostream>
#include <string>

Q_DECLARE_OPAQUE_POINTER(const VSFrame *)
Q_DECLARE_OPAQUE_POINTER(VSNode *)

SettingsManager * pSettings = nullptr;
VSScriptLibrary * pVSSLibrary = nullptr;
EncodeDialog * pEncodeDialog = nullptr;
int exitCode = 0;

void writeLogMessageByTypename(const QString & a_msg, const QString & a_style)
{
	QString debugTypes[] = {
		LOG_STYLE_DEBUG,
		LOG_STYLE_QT_DEBUG,
		LOG_STYLE_VS_DEBUG,
	};

	if(pSettings->getShowDebugMessages() || !vsedit::contains(debugTypes, a_style))
	{
		std::cerr << "[" << a_style.toUpper().toStdString() << "] "
			<< std::string(a_msg.toLocal8Bit()) << std::endl;
	}

	QString breakingTypes[] = {
		LOG_STYLE_VS_CRITICAL,
		LOG_STYLE_QT_CRITICAL,
		LOG_STYLE_ERROR,
		LOG_STYLE_VS_FATAL,
		LOG_STYLE_QT_FATAL,
	};

	if(vsedit::contains(breakingTypes, a_style))
	{
		if(pEncodeDialog)
		{
			pEncodeDialog->show();
			QMessageBox * msgBox = new QMessageBox(pEncodeDialog);
			msgBox->setText(a_msg);
			msgBox->setWindowTitle(a_style.toUpper());
			vsedit::disableFontKerning(msgBox);
			msgBox->setTextInteractionFlags(Qt::TextSelectableByMouse);
			msgBox->exec();
		}

		// Handle fatal errors and save to current dir
		QString fatalTypes[] = {LOG_STYLE_VS_FATAL, LOG_STYLE_QT_FATAL};
		if(vsedit::contains(fatalTypes, a_style))
		{
			QDateTime now = QDateTime::currentDateTime();
			QString timeString = now.toString("hh:mm:ss.zzz");
			QString dateString = now.toString("yyyy-MM-dd");
			QString caption = QObject::tr("vsedit-encode fatal error!");
			QString fullMessage = dateString + QString(" ") + timeString +
				QString("\n") + caption + QString("\n") + a_msg;

			QString applicationDir = QCoreApplication::applicationDirPath();
			QString errorLogFilePath = applicationDir + QString("/") +
				QString("vsedit-encode-crashlog-") +	dateString + QString("-") +
				timeString.replace(':', '-') + QString(".txt");

			QFile errorLogFile(errorLogFilePath);
			if(errorLogFile.open(QIODevice::WriteOnly))
			{
				errorLogFile.write(fullMessage.toUtf8());
				errorLogFile.close();
			}
		}

		qApp->exit(-1);
		exitCode = -1;
	}
}

void writeLogMessage(int a_msgType, const QString & a_msg)
{
	QString style = vsMessageTypeToStyleName(a_msgType);
	writeLogMessageByTypename(a_msg, style);
}

void handleQtMessage(QtMsgType a_type,
	const QMessageLogContext & a_context, const QString & a_message)
{
	QString style = LOG_STYLE_DEFAULT;

	switch(a_type)
	{
	case QtDebugMsg:
		style = LOG_STYLE_QT_DEBUG;
		break;
	case QtInfoMsg:
		style = LOG_STYLE_QT_INFO;
		break;
	case QtWarningMsg:
		style = LOG_STYLE_QT_WARNING;
		break;
	case QtCriticalMsg:
		style = LOG_STYLE_QT_CRITICAL;
		break;
	case QtFatalMsg:
		style = LOG_STYLE_QT_FATAL;
		break;
	default:
		Q_ASSERT(false);
	}

	QString fullMessage = QString("%1: %2").arg(style.toUpper()).arg(a_message);

	QString fileString(a_context.file);
	QString lineString = QString::number(a_context.line);
	QString functionString(a_context.function);

	QString lineInfo = QString("\n(%1:%2").arg(fileString).arg(lineString);
	if(!functionString.isEmpty())
		lineInfo += QString(", %1").arg(functionString);
	lineInfo += QString(")");
	if(!fileString.isEmpty())
		fullMessage += lineInfo;

	writeLogMessageByTypename(fullMessage, style);
}

int main(int argc, char *argv[])
{
	QString scriptFilePath;
	if(argc > 1)
	{
		scriptFilePath = QString::fromLocal8Bit(argv[1], -1);
	}
	else
	{
		std::cerr << "vsedit-encode: Please provide the path to your script." << std::endl;
		return -2;
	}

	QApplication::setAttribute(Qt::AA_DontUseNativeMenuBar);
	QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
		Qt::HighDpiScaleFactorRoundingPolicy::Floor);
	QApplication application(argc, argv);

	qInstallMessageHandler(handleQtMessage);

	pSettings = new SettingsManager(qApp);

	vsedit::disableFontKerning(qApp);

	// Make text in message box selectable
	application.setStyleSheet(
		"QToolTip { font-kerning: none; }"
		"QMessageBox { messagebox-text-interaction-flags: 5; }"
		"QLabel { padding: 0px; }");

	qRegisterMetaType<const VSFrame *>("const VSFrame *");
	qRegisterMetaType<VSNode *>("VSNode *");

	QResource digitalMiniFontResource(":/fonts/DigitalMini.ttf");
	QByteArray digitalMiniFontData(
		(const char *)digitalMiniFontResource.data(),
		digitalMiniFontResource.size());
	QFontDatabase::addApplicationFontFromData(digitalMiniFontData);

	if(pSettings->inDarkMode())
	{
		// Load qDarkStyle colors
		QFile styleSheetDark(":/dark/style.qss");
		if(!styleSheetDark.open(QFile::ReadOnly | QFile::Text))
		{
			QMessageBox::critical(nullptr,
				QString::fromUtf8("File open error"),
				QString::fromUtf8("Failed to open stylesheet file ")
					+ styleSheetDark.errorString());
		}
		qApp->setStyleSheet(styleSheetDark.readAll());
		QPalette newPal(qApp->palette());
		newPal.setColor(QPalette::Base, QColor(0, 0, 0));
		newPal.setColor(QPalette::Highlight, QColor(128, 128, 128));
		newPal.setColor(QPalette::Dark, QColor(192, 192, 192));
		newPal.setColor(QPalette::Text, QColor(64, 192, 0));
		qApp->setPalette(newPal);
	}
#ifdef Q_OS_WIN
	else
		qApp->setStyle("fusion");
#endif

	QFileInfo fileInfo(scriptFilePath);
	QString scriptFileFullPath = fileInfo.absoluteFilePath();
	QFile scriptFile(scriptFileFullPath);
	bool loaded = scriptFile.open(QIODevice::ReadOnly | QIODevice::Text);
	if(!loaded)
	{
		QString errorMsg = QString("Failed to open script [%1]!")
			.arg(scriptFilePath);
		writeLogMessageByTypename(errorMsg, LOG_STYLE_ERROR);
		delete pSettings;
		return -1;
	}

	pVSSLibrary = new VSScriptLibrary(pSettings, qApp);
	QObject::connect(pVSSLibrary, &VSScriptLibrary::signalWriteLogMessage,
		writeLogMessage);

    pEncodeDialog = new EncodeDialog(pSettings, pVSSLibrary, nullptr);
    QObject::connect(pEncodeDialog, &EncodeDialog::signalWriteLogMessage,
        writeLogMessageByTypename);

	if(exitCode != 0)
		return exitCode;

	QString scriptText = QString::fromUtf8(scriptFile.readAll());
	pSettings->setLastUsedPath(scriptFileFullPath);

	if(pVSSLibrary->initialize())
	{
		if(pEncodeDialog->initialize(scriptText, scriptFilePath))
		{
			pEncodeDialog->showActive();
			exitCode = pEncodeDialog->exec();
		}
	}
	return exitCode;
}
