/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "BloomJS.h"

#include "BaseObjectJS.h"

template<napi_value (BloomConfiguration::*F)(NapiApi::FunctionContext<>&)>
napi_value BloomConfiguration::Method(napi_env env, napi_callback_info info)
{
    NapiApi::FunctionContext fc(env, info);
    if (fc && fc.This()) {
        BloomConfiguration* impl = nullptr;
        napi_unwrap(fc.GetEnv(), fc.This().ToNapiValue(), (void**)&impl);
        if (impl) {
            return (impl->*F)(fc);
        }
    }
    return fc.GetUndefined();
}

template<napi_value (BloomConfiguration::*F)(NapiApi::FunctionContext<>&)>
napi_value BloomConfiguration::Getter(napi_env env, napi_callback_info info)
{
    NapiApi::FunctionContext fc(env, info);
    if (fc && fc.This()) {
        BloomConfiguration* impl = nullptr;
        napi_unwrap(fc.GetEnv(), fc.This().ToNapiValue(), (void**)&impl);
        if (impl) {
            return (impl->*F)(fc);
        }
    }
    return fc.GetUndefined();
}
template<typename Type, void (BloomConfiguration::*F)(NapiApi::FunctionContext<Type>&)>
inline napi_value BloomConfiguration::Setter(napi_env env, napi_callback_info info)
{
    NapiApi::FunctionContext<Type> fc(env, info);
    if (fc && fc.This()) {
        BloomConfiguration* impl = nullptr;
        napi_unwrap(fc.GetEnv(), fc.This().ToNapiValue(), (void**)&impl);
        if (impl) {
            (impl->*F)(fc);
        }
    }
    return fc.GetUndefined();
};

BloomConfiguration::BloomConfiguration() {}

void BloomConfiguration::Detach()
{
    if (bloom_ || postproc_) {
        SCENE_NS::IBloom::WeakPtr tmpb = bloom_;
        SCENE_NS::IPostProcess::WeakPtr tmpp = postproc_;
        if (bloom_) {
            bloom_.reset(); // release it.
            //* can verify if the object actually died by trying to lock it.
            if (!tmpb.lock()) {
                LOG_V("bloom instance destroyed");
            }
        }
        if (postproc_) {
            postproc_.reset(); // release it.
            if (!tmpp.lock()) {
                LOG_V("postprocess instance destroyed");
            }
        }
    }
}
BloomConfiguration::~BloomConfiguration()
{
    if (bloom_ || postproc_) {
        Detach();
    }
}
void BloomConfiguration::SetFrom(NapiApi::Object obj)
{
    type_ = (Type)obj.Get<uint32_t>("type").valueOrDefault((uint32_t)Type::NORMAL);
    quality_ = (Quality)obj.Get<uint32_t>("quality").valueOrDefault((uint32_t)Quality::NORMAL);
    thresholdHard_ = obj.Get<float>("thresholdHard").valueOrDefault(1.0);
    thresholdSoft_ = obj.Get<float>("thresholdSoft").valueOrDefault(2.0); // 2.0: param
    scatter_ = obj.Get<float>("scatter").valueOrDefault(1.0);
    scaleFactor_ = obj.Get<float>("scaleFactor").valueOrDefault(1.0);
    amountCoefficient_ = obj.Get<float>("amountCoefficient").valueOrDefault(0.25f);
}
void BloomConfiguration::SetTo(NapiApi::Object obj)
{
    NapiApi::Env env(obj.GetEnv());
    obj.Set("type", env.GetNumber((uint32_t)type_));
    obj.Set("quality", env.GetNumber((uint32_t)quality_));
    obj.Set("thresholdHard", env.GetNumber(thresholdHard_));
    obj.Set("thresholdSoft", env.GetNumber(thresholdSoft_));
    obj.Set("scatter", env.GetNumber(scatter_));
    obj.Set("scaleFactor", env.GetNumber(scaleFactor_));
    obj.Set("amountCoefficient", env.GetNumber(amountCoefficient_));
}

void BloomConfiguration::SetFrom(SCENE_NS::IBloom::Ptr bloom)
{
    ExecSyncTask([bloom, this]() {
        type_ = SetType(bloom->Type()->GetValue());
        quality_ = SetQuality(bloom->Quality()->GetValue());
        thresholdHard_ = bloom->ThresholdHard()->GetValue();
        thresholdSoft_ = bloom->ThresholdSoft()->GetValue();
        scatter_ = bloom->Scatter()->GetValue();
        scaleFactor_ = bloom->ScaleFactor()->GetValue();
        amountCoefficient_ = bloom->AmountCoefficient()->GetValue();
        return META_NS::IAny::Ptr {};
    });
}

void BloomConfiguration::SetTo(SCENE_NS::IBloom::Ptr bloom)
{
    ExecSyncTask([bloom, this]() {
        bloom->Enabled()->SetValue(true);
        bloom->Type()->SetValue(GetType(type_));
        bloom->Quality()->SetValue(GetQuality(quality_));
        bloom->ThresholdHard()->SetValue(thresholdHard_);
        bloom->ThresholdSoft()->SetValue(thresholdSoft_);
        bloom->Scatter()->SetValue(scatter_);
        bloom->ScaleFactor()->SetValue(scaleFactor_);
        bloom->AmountCoefficient()->SetValue(amountCoefficient_);
        if (postproc_) {
            postproc_->Bloom()->SetValue(bloom);
        }
        return META_NS::IAny::Ptr {};
    });
}

BloomConfiguration* BloomConfiguration::Unwrap(NapiApi::Object obj)
{
    BloomConfiguration* settings = nullptr;
    napi_unwrap(obj.GetEnv(), obj.ToNapiValue(), (void**)&settings);
    return settings;
}

napi_value BloomConfiguration::Dispose(NapiApi::FunctionContext<>& ctx)
{
    Detach();
    return ctx.GetUndefined();
}

NapiApi::StrongRef BloomConfiguration::Wrap(NapiApi::Object obj)
{
    //  wrap it..
    auto DTOR = [](napi_env env, void* nativeObject, void* finalize) {
        BloomConfiguration* ptr = static_cast<BloomConfiguration*>(nativeObject);
        delete ptr;
    };

    napi_wrap(obj.GetEnv(), obj.ToNapiValue(), this, DTOR, nullptr, nullptr);

    // clang-format off
    napi_property_descriptor descs[] = {
        { "type", nullptr, nullptr, Getter<&BloomConfiguration::GetType>,
            Setter<uint32_t, &BloomConfiguration::SetType>, nullptr, napi_default_jsproperty, this },
        { "quality", nullptr, nullptr, Getter<&BloomConfiguration::GetQuality>,
            Setter<uint32_t, &BloomConfiguration::SetQuality>, nullptr, napi_default_jsproperty, this },
        { "amountCoefficient", nullptr, nullptr, Getter<&BloomConfiguration::GetAmount>,
            Setter<float, &BloomConfiguration::SetAmount>, nullptr, napi_default_jsproperty, this },
        { "thresholdSoft", nullptr, nullptr, Getter<&BloomConfiguration::GetThresholdSoft>,
            Setter<float, &BloomConfiguration::SetThresholdSoft>, nullptr, napi_default_jsproperty, this },
        { "thresholdHard", nullptr, nullptr, Getter<&BloomConfiguration::GetThresholdHard>,
            Setter<float, &BloomConfiguration::SetThresholdHard>, nullptr, napi_default_jsproperty, this },
        { "scatter", nullptr, nullptr, Getter<&BloomConfiguration::GetScatter>,
            Setter<float, &BloomConfiguration::SetScatter>, nullptr, napi_default_jsproperty, this },
        { "scaleFactor", nullptr, nullptr, Getter<&BloomConfiguration::GetScaleFactor>,
            Setter<float, &BloomConfiguration::SetScaleFactor>, nullptr, napi_default_jsproperty, this },
        { "destroy", nullptr, Method<&BloomConfiguration::Dispose>,
            nullptr, nullptr, nullptr, napi_default_jsproperty, this }
    };
    // clang-format on
    napi_define_properties(obj.GetEnv(), obj.ToNapiValue(), BASE_NS::countof(descs), descs);

    SetTo(obj);
    return NapiApi::StrongRef(obj);
}

napi_value BloomConfiguration::GetType(NapiApi::FunctionContext<>& ctx)
{
    SCENE_NS::BloomType type = GetType(type_);
    if (bloom_) {
        type = bloom_->Type()->GetValue();
    }
    uint32_t tp = (uint32_t)SetType(type);
    return ctx.GetNumber(tp);
}
napi_value BloomConfiguration::GetQuality(NapiApi::FunctionContext<>& ctx)
{
    SCENE_NS::EffectQualityType type = GetQuality(quality_);
    if (bloom_) {
        type = bloom_->Quality()->GetValue();
    }
    uint32_t tp = (uint32_t)SetQuality(type);
    return ctx.GetNumber(tp);
}

napi_value BloomConfiguration::GetScatter(NapiApi::FunctionContext<>& ctx)
{
    float amount = scatter_;
    if (bloom_) {
        ExecSyncTask([bl = bloom_, &amount]() {
            amount = bl->Scatter()->GetValue();
            return META_NS::IAny::Ptr {};
        });
    }
    return ctx.GetNumber(amount);
}

napi_value BloomConfiguration::GetScaleFactor(NapiApi::FunctionContext<>& ctx)
{
    float amount = scaleFactor_;
    if (bloom_) {
        ExecSyncTask([bl = bloom_, &amount]() {
            amount = bl->ScaleFactor()->GetValue();
            return META_NS::IAny::Ptr {};
        });
    }
    return ctx.GetNumber(amount);
}

napi_value BloomConfiguration::GetThresholdHard(NapiApi::FunctionContext<>& ctx)
{
    float amount = thresholdHard_;
    if (bloom_) {
        amount = bloom_->ThresholdHard()->GetValue();
    }
    return ctx.GetNumber(amount);
}

napi_value BloomConfiguration::GetThresholdSoft(NapiApi::FunctionContext<>& ctx)
{
    float amount = thresholdSoft_;
    if (bloom_) {
        amount = bloom_->ThresholdSoft()->GetValue();
    }
    return ctx.GetNumber(amount);
}

napi_value BloomConfiguration::GetAmount(NapiApi::FunctionContext<>& ctx)
{
    float amount = amountCoefficient_;
    if (bloom_) {
        amount = bloom_->AmountCoefficient()->GetValue();
    }
    return ctx.GetNumber(amount);
}
void BloomConfiguration::SetType(NapiApi::FunctionContext<uint32_t>& ctx)
{
    type_ = (BloomConfiguration::Type)ctx.Arg<0>().valueOrDefault();
    if (bloom_) {
        bloom_->Type()->SetValue(GetType(type_));
        if (postproc_) {
            postproc_->Bloom()->SetValue(bloom_);
        }
    }
}
void BloomConfiguration::SetQuality(NapiApi::FunctionContext<uint32_t>& ctx)
{
    quality_ = (BloomConfiguration::Quality)ctx.Arg<0>().valueOrDefault();
    if (bloom_) {
        bloom_->Quality()->SetValue(GetQuality(quality_));
        if (postproc_) {
            postproc_->Bloom()->SetValue(bloom_);
        }
    }
}
void BloomConfiguration::SetThresholdHard(NapiApi::FunctionContext<float>& ctx)
{
    thresholdHard_ = ctx.Arg<0>();
    if (bloom_) {
        bloom_->ThresholdHard()->SetValue(thresholdHard_);
        if (postproc_) {
            postproc_->Bloom()->SetValue(bloom_);
        }
    }
}
void BloomConfiguration::SetScatter(NapiApi::FunctionContext<float>& ctx)
{
    scatter_= ctx.Arg<0>();
    if (bloom_) {
        bloom_->Scatter()->SetValue(scatter_);
        if (postproc_) {
            postproc_->Bloom()->SetValue(bloom_);
        }
    }
}
void BloomConfiguration::SetScaleFactor(NapiApi::FunctionContext<float>& ctx)
{
    scaleFactor_ = ctx.Arg<0>();
    if (bloom_) {
        bloom_->ScaleFactor()->SetValue(scaleFactor_);
        if (postproc_) {
            postproc_->Bloom()->SetValue(bloom_);
        }
    }
}
void BloomConfiguration::SetThresholdSoft(NapiApi::FunctionContext<float>& ctx)
{
    thresholdSoft_ = ctx.Arg<0>();
    if (bloom_) {
        bloom_->ThresholdSoft()->SetValue(thresholdSoft_);
        if (postproc_) {
            postproc_->Bloom()->SetValue(bloom_);
        }
    }
}
void BloomConfiguration::SetAmount(NapiApi::FunctionContext<float>& ctx)
{
    amountCoefficient_ = ctx.Arg<0>();
    if (bloom_) {
        bloom_->AmountCoefficient()->SetValue(amountCoefficient_);
        if (postproc_) {
            postproc_->Bloom()->SetValue(bloom_);
        }
    }
}
SCENE_NS::IPostProcess::Ptr BloomConfiguration::GetPostProc()
{
    return postproc_;
}

void BloomConfiguration::SetPostProc(SCENE_NS::IPostProcess::Ptr postproc)
{
    if (!postproc) {
        // detaching.
        Detach();
        return;
    }
    if ((postproc_) && (postproc != postproc_)) {
        LOG_F("Invalid state");
        return;
    }
    postproc_ = postproc;
    auto bloom = postproc_->Bloom()->GetValue();
    if ((bloom_) && (bloom != bloom_)) {
        LOG_F("Invalid state");
        bloom = nullptr;
    } else {
        if (!bloom) {
            bloom = META_NS::GetObjectRegistry().Create<SCENE_NS::IBloom>(SCENE_NS::ClassId::Bloom);
        }
        bloom_ = bloom;
    }
    if (!bloom_) {
        postproc_ = nullptr;
    } else {
        SetTo(bloom_);
    }
}

SCENE_NS::EffectQualityType BloomConfiguration::GetQuality(BloomConfiguration::Quality bloomQualityType)
{
    switch (bloomQualityType) {
        case Quality::LOW:
            return SCENE_NS::EffectQualityType::LOW;
        case Quality::HIGH:
            return SCENE_NS::EffectQualityType::HIGH;
        case Quality::NORMAL:
        default:
            return SCENE_NS::EffectQualityType::NORMAL;
    };
    return SCENE_NS::EffectQualityType::NORMAL;
}
BloomConfiguration::Quality BloomConfiguration::SetQuality(SCENE_NS::EffectQualityType bloomQualityType)
{
    switch (bloomQualityType) {
        case SCENE_NS::EffectQualityType::LOW:
            return BloomConfiguration::Quality::LOW;
        case SCENE_NS::EffectQualityType::HIGH:
            return BloomConfiguration::Quality::HIGH;
        case SCENE_NS::EffectQualityType::NORMAL:
        default:
            return BloomConfiguration::Quality::NORMAL;
    };
    return BloomConfiguration::Quality::NORMAL;
}

SCENE_NS::BloomType BloomConfiguration::GetType(BloomConfiguration::Type bloomType)
{
    switch (bloomType) {
        case Type::HORIZONTAL:
            return SCENE_NS::BloomType::HORIZONTAL;
        case Type::VERTICAL:
            return SCENE_NS::BloomType::VERTICAL;
        case Type::BILATERAL:
            return SCENE_NS::BloomType::BILATERAL;
        case Type::NORMAL:
        default:
            return SCENE_NS::BloomType::NORMAL;
    };
    return SCENE_NS::BloomType::NORMAL;
}
BloomConfiguration::Type BloomConfiguration::SetType(SCENE_NS::BloomType bloomTypeIn)
{
    switch (bloomTypeIn) {
        case SCENE_NS::BloomType::HORIZONTAL:
            return BloomConfiguration::Type::HORIZONTAL;
        case SCENE_NS::BloomType::VERTICAL:
            return BloomConfiguration::Type::VERTICAL;
        case SCENE_NS::BloomType::BILATERAL:
            return BloomConfiguration::Type::NORMAL;
        case SCENE_NS::BloomType::NORMAL:
        default:
            return BloomConfiguration::Type::NORMAL;
    };
    return BloomConfiguration::Type::NORMAL;
}
