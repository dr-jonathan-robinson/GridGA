#pragma once

#include "stdafx.hpp"

namespace GridGALib
{
    enum ParameterType
    {
        PARAMETER_TYPE_INTEGER,
        PARAMETER_TYPE_EXP_2,
        PARAMETER_TYPE_CATEGORICAL
    };


    class GenomeParameter
    {
    public:
        virtual boost::int32_t InternalValue(void) const = 0;
        virtual std::string GetValueForConfig(void) const = 0;
        virtual void Decrease(void) = 0;
        virtual void Increase(void) = 0;
        virtual void SetRandomValue(void) = 0;
        virtual void SetInternalValue(boost::int32_t newValue) = 0;
        virtual ParameterType GetParameterType(void) const = 0;

        GenomeParameter(std::string identifier)
        :
            mHasValue(true),
            mIdentifier(identifier)
        {}

        GenomeParameter()
        :
            mHasValue(false)
        {}

        std::string GetIdentifier(void) const
        {
            return mIdentifier;
        }

        bool GetHasValue(void) const
        {
            return mHasValue;
        }
    protected:
        bool mHasValue;
        bool mSingleton;
        std::string mIdentifier;
    };

    class GenomeParameterContinuous : public GenomeParameter
    {
    public:
        GenomeParameterContinuous(std::string identifier, boost::int32_t minimumValue, boost::int32_t maximumValue, boost::int32_t step) 
        : 
            GenomeParameter(identifier),
            mValue((maximumValue - minimumValue) / 2),
            mMinimum(minimumValue),
            mMax(maximumValue),
            mStep(step)
        {}

        GenomeParameterContinuous() : GenomeParameter()
        {}

        GenomeParameterContinuous(GenomeParameterContinuous& existingGenome)
        :
            GenomeParameter(existingGenome.GetIdentifier()),
            mValue(existingGenome.mValue),
            mMinimum(existingGenome.mMinimum),
            mMax(existingGenome.mMax),
            mStep(existingGenome.mStep)
        {}

        boost::int32_t InternalValue(void) const override
        {
            return mValue;
        }

        virtual std::string GetValueForConfig(void) const override
        {
            return std::to_string(mValue);
        }

        void Decrease(void) override
        {
            mValue -= mStep;
            if (mValue < mMinimum)
            {
                mValue = mMinimum;
            }
        }

        void Increase(void) override
        {
            mValue += mStep;
            if (mValue > mMax)
            {
                mValue = mMax;
            }
        }

        void SetInternalValue(boost::int32_t newValue) override
        {
            mValue = newValue;
        }

        virtual ParameterType GetParameterType(void) const override
        {
            return PARAMETER_TYPE_INTEGER;
        }

        void SetRandomValue(void) override
        {
            mValue = mMinimum + (rand() % ((mMax+1) - mMinimum));
            mValue = (static_cast<boost::int32_t>(std::floor(static_cast<double>(mValue)/static_cast<double>(mStep))) * mStep);
        }
     
    protected:
        boost::int32_t mValue;
        boost::int32_t mMinimum;
        boost::int32_t mMax;
        boost::int32_t mStep;
    };

    class GenomeParameterExp2 : public GenomeParameterContinuous
    {
    public:
        GenomeParameterExp2(std::string identifier, boost::int32_t minimumValue, boost::int32_t maximumValue, boost::int32_t step) 
        : 
            GenomeParameterContinuous(identifier, minimumValue, maximumValue, step)
        {}
  

        std::string GetValueForConfig(void) const override
        {
            return std::to_string(std::pow(2.0, mValue));
        }

        ParameterType GetParameterType(void) const override
        {
            return PARAMETER_TYPE_EXP_2;
        }
    };

    class GenomeParameterCategorical : public GenomeParameter
    {
    public:
        GenomeParameterCategorical(std::string identifier, std::string csvCategories) 
        : 
            GenomeParameter(identifier)
        {
            boost::split(mCategories, csvCategories, boost::is_any_of(", :;|"));
        }

        GenomeParameterCategorical() : GenomeParameter()
        {}

        GenomeParameterCategorical(GenomeParameterCategorical& existingGenome)
        :
            GenomeParameter(existingGenome.GetIdentifier()),
            mCategories(existingGenome.mCategories)
        {}

        boost::int32_t InternalValue(void) const override
        {
            return mIndex;
        }

        virtual std::string GetValueForConfig(void) const override
        {
            return mCategories[mIndex];
        }

        void Decrease(void) override
        {
            if (mIndex > 0)
            {
                --mIndex;
            }
        }

        void Increase(void) override
        {
            if (mIndex < static_cast<boost::int32_t>(mCategories.size()-1))
            {
                ++mIndex;
            }
        }

        void SetInternalValue(boost::int32_t newValue) override
        {
            mIndex = newValue;
        }

        virtual ParameterType GetParameterType(void) const override
        {
            return PARAMETER_TYPE_CATEGORICAL;
        }

        void SetRandomValue(void) override
        {
            mIndex = rand() % mCategories.size();
        }
     
    protected:
        boost::int32_t mIndex;
        std::vector<std::string> mCategories;
    };

    typedef boost::shared_ptr<GenomeParameter> GenomeParameterPtr;  // use pointers as we need to 'create' the abstract GenomeParameter class.
    typedef std::map<std::string, GenomeParameterPtr> GAParameterMap;
    typedef boost::shared_ptr<GAParameterMap> GAParameterMapPtr;

    template <typename T>
    GenomeParameterPtr GetGenomeParameterCopy(GenomeParameterPtr genomeToCopy)
    {
        T& genomeParameterOriginal(*dynamic_cast<T*>(genomeToCopy.get()));
        GenomeParameterPtr genomeCopy(boost::make_shared<T>(genomeParameterOriginal));
        return genomeCopy;
    }

    class Genome
    {
    public:
        Genome(void);
        void SetIntParameterList(GAParameterMapPtr parameters);
        void SetRandomValues();
        void LoadFromXML(const boost::property_tree::ptree& pt);
        void SetInternalParameterValue(const std::string& parameterID, boost::int32_t value);
        boost::int32_t GetInternalParameterValue(const std::string& parameterId);      
        double GetObjective(void) const;
        std::size_t GetGenomeID(void) const;
        void SaveAsXML(boost::property_tree::ptree& genomeTree) const;
        void Update(const boost::property_tree::ptree& pt);
        bool Mutate(std::size_t mutationProbability);
        std::string ToString(void) const;
        GAParameterMapPtr GetParameters() const;
        bool IsComplete(void) const;
        std::string GetCommandLineArguments(const std::string& paramPrefix, const std::string& valuePrefix) const;

        static void SetGenerationNumber(boost::int32_t generationNumber);
    private:
        GAParameterMapPtr mParameters;
        std::size_t mGenomeID;
        bool mComplete;
        boost::int32_t mPriceMoveTarget;
        double mObjective;
        std::string mComputeHost;
        static std::size_t GenomeID;
    };

    typedef boost::shared_ptr<Genome> GenomePtr;
    typedef boost::shared_ptr<std::deque<GenomePtr> > GenomeList;

    inline bool CompareGenomeByObjective(GenomePtr i, GenomePtr j)
    {
        return (i->GetObjective() > j->GetObjective());
    }

}
