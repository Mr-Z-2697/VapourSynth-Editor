#ifndef RESAMPLE_H_INCLUDED
#define RESAMPLE_H_INCLUDED

#include "../common/aligned_vector.h"
#include "../zimg/API/zimg.h"

#include <QString>

namespace vsedit
{

	class Resampler
	{
		public:

			Resampler();
			virtual ~Resampler();

			void clear();

			bool resample(
				const void * a_pSource,
				int a_sourceWidth,
				int a_sourceHeight,
				int a_sourceStride,
				void * a_pDestination,
				int a_destinationWidth,
				int a_destinationHeight,
				int a_destinationStride,
				int a_pixelType,
				double a_shiftW,
				double a_shiftH,
				int a_filterType,
				double a_filterParamA,
				double a_filterParamB,
				float a_clampMin,
				float a_clampMax);

			const QString & getError() const;

		private:

			int m_sourceWidth;
			int m_sourceHeight;
			int m_destinationWidth;
			int m_destinationHeight;
			int m_pixelType;
			double m_shiftW;
			double m_shiftH;
			int m_filterType;
			double m_filterParamA;
			double m_filterParamB;
			zimg_resize_context * m_pResizeContext;

			vsedit::aligned_vector<uint8_t> m_inFloatBuffer;
			vsedit::aligned_vector<uint8_t> m_outFloatBuffer;
			vsedit::aligned_vector<uint8_t> m_resizeTempBuffer;

			QString m_error;

	};

}

#endif // RESAMPLE_H_INCLUDED
