#ifndef VAPOURSYNTHSCRIPTPROCESSOR_H
#define VAPOURSYNTHSCRIPTPROCESSOR_H

#include <QObject>
#include <QPixmap>
#include <QLibrary>
#include <QMutex>
#include <vapoursynth/VSScript.h>
#include <deque>
#include <set>

#include "../settings/settingsmanager.h"

//==============================================================================

typedef int (VS_CC *FNP_vssInit)(void);
typedef const VSAPI * (VS_CC *FNP_vssGetVSApi)(void);
typedef int (VS_CC *FNP_vssEvaluateScript)(VSScript ** a_handle,
	const char * a_script, const char * a_scriptFilename, int a_flags);
typedef const char * (VS_CC *FNP_vssGetError)(VSScript * a_handle);
typedef VSCore * (VS_CC *FNP_vssGetCore)(VSScript * a_handle);
typedef VSNodeRef * (VS_CC *FNP_vssGetOutput)(VSScript * a_handle,
	int a_index);
typedef void (VS_CC *FNP_vssFreeScript)(VSScript * a_handle);
typedef int (VS_CC *FNP_vssFinalize)(void);

//==============================================================================

struct FrameTicket
{
	int frameNumber;
	VSNodeRef * pNode;
	VSFrameDoneCallback fpCallback;

	FrameTicket(int a_frameNumber, VSNodeRef * a_pNode,
		VSFrameDoneCallback a_fpCallback);
	bool operator<(const FrameTicket & a_other) const;
	bool operator==(const FrameTicket & a_other) const;
};

//==============================================================================

class VapourSynthScriptProcessor : public QObject
{
	Q_OBJECT

	public:

		VapourSynthScriptProcessor(SettingsManager * a_pSettingsManager,
			QObject * a_pParent = nullptr);

		virtual ~VapourSynthScriptProcessor();

		bool initialize(const QString& a_script, const QString& a_scriptName);

		bool finalize();

		bool isInitialized() const;

		QString error() const;

		const VSVideoInfo * videoInfo();

		bool requestFrame(int a_frameNumber);

		void freeFrame();

		QPixmap pixmap(int a_frameNumber);

		void requestPixmapAsync(int a_frameNumber);

		void colorAtPoint(size_t a_x, size_t a_y, double & a_rValue1,
			double & a_rValue2, double & a_rValue3);

	signals:

		void signalWriteLogMessage(int a_messageType,
			const QString & a_message);

		void signalDistributePixmap(int a_frameNumber,
			const QPixmap & a_pixmap);

		void signalFrameQueStateChanged(size_t a_inQue, size_t a_inProcess,
			size_t a_maxThreads);

	private slots:

		void slotSettingsChanged();

	private:

		void handleVSMessage(int a_messageType, const QString & a_message);

		void receiveFrameForPreview(const VSFrameRef * a_cpFrameRef,
			int a_frameNumber, VSNodeRef * a_pNodeRef,
			const QString & a_errorMessage);

		void receiveFrameForPreviewAndProcessQue(
			const VSFrameRef * a_cpFrameRef, int a_frameNumber,
			VSNodeRef * a_pNodeRef, const QString & a_errorMessage);

		QPixmap pixmapFromFrame(const VSFrameRef * a_cpFrameRef);

		bool initLibrary();

		void freeLibrary();

		double valueAtPoint(size_t a_x, size_t a_y, int a_plane);

		void initPreviewNode();

		void requestFrameAsync(int a_frameNumber, VSNodeRef * a_pNodeRef,
			VSFrameDoneCallback a_fpCallback);

		void processFrameTicketsQue();

		void sendFrameQueChangeSignal();

		friend void VS_CC vsMessageHandler(int a_msgType,
			const char * a_message, void * a_pUserData);

		friend void VS_CC frameForPreviewReady(void * a_pUserData,
			const VSFrameRef * a_cpFrameRef, int a_frameNumber,
			VSNodeRef * a_pNodeRef, const char * a_errorMessage);

		SettingsManager * m_pSettingsManager;

		QString m_script;

		QString m_scriptName;

		QString m_error;

		bool m_vsScriptInitialized;

		bool m_initialized;
		const VSAPI * m_cpVSAPI;

		VSScript * m_pVSScript;

		VSNodeRef * m_pOutputNode;
		VSNodeRef * m_pPreviewNode;

		const VSVideoInfo * m_cpVideoInfo;
		const VSCoreInfo * m_cpCoreInfo;

		int m_currentFrame;

		int m_lastRequestedPixmapFrameNumber;

		const VSFrameRef * m_cpCurrentFrameRef;

		ResamplingFilter m_chromaResamplingFilter;

		ChromaPlacement m_chromaPlacement;

		double m_resamplingFilterParameterA;

		double m_resamplingFilterParameterB;

		YuvToRgbConversionMatrix m_yuvMatrix;

		QLibrary m_vsScriptLibrary;

		FNP_vssInit vssInit;
		FNP_vssGetVSApi vssGetVSApi;
		FNP_vssEvaluateScript vssEvaluateScript;
		FNP_vssGetError vssGetError;
		FNP_vssGetCore vssGetCore;
		FNP_vssGetOutput vssGetOutput;
		FNP_vssFreeScript vssFreeScript;
		FNP_vssFinalize vssFinalize;

		std::deque<FrameTicket> m_frameTicketsQue;
		std::multiset<FrameTicket> m_frameTicketsInProcess;

		QMutex m_framesQueMutex;
};

//==============================================================================

#endif // VAPOURSYNTHSCRIPTPROCESSOR_H
