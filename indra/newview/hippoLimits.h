#ifndef __HIPPO_LIMITS_H__
#define __HIPPO_LIMITS_H__


class HippoLimits
{
public:
	HippoLimits();

	int   getMaxAgentGroups() const { return mMaxAgentGroups; }
	float getMaxHeight()      const { return mMaxHeight;      }
	float getMinHoleSize()    const { return mMinHoleSize;    }
	float getMaxHollow()      const { return mMaxHollow;      }
	float getMaxPrimScale()   const { return mMaxPrimScale;   }

	void setLimits();
	void setOpenSimLimits();
	void setSecondLifeLimits();

private:
	int   mMaxAgentGroups;
	float mMaxHeight;
	float mMinHoleSize;
	float mMaxHollow;
	float mMaxPrimScale;

};


extern HippoLimits *gHippoLimits;


#endif
