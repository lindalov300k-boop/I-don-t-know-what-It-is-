#pragma once
// ==============================================================================
// Animator.h — ECS component + AnimatorSystem
// Each entity with an Animator owns an AnimStateMachine and drives a SpriteComponent.
// ==============================================================================

#include "statemachine/AnimStateMachine.h"
#include "../ecs/system/World.h"
#include "../ecs/component/Components.h"
#include "../engine/ModuleSystem/ModuleInterface.h"
#include <memory>
#include <string>
#include <functional>

namespace BADGE2D {

// ==============================================================================
// AnimatorComponent — ECS component
// ==============================================================================
struct AnimatorComponent {
    std::unique_ptr<AnimStateMachine> stateMachine;

    // Convenience pass-throughs to the state machine
    void SetFloat  (const std::string& p, float v)  { if (stateMachine) stateMachine->SetFloat(p, v); }
    void SetBool   (const std::string& p, bool v)   { if (stateMachine) stateMachine->SetBool(p, v); }
    void SetInt    (const std::string& p, int v)     { if (stateMachine) stateMachine->SetInt(p, v); }
    void SetTrigger(const std::string& p)            { if (stateMachine) stateMachine->SetTrigger(p); }
    void Play      (const std::string& state)        { if (stateMachine) stateMachine->ForceState(state); }

    bool IsInState (const std::string& s) const { return stateMachine && stateMachine->IsInState(s); }
    const std::string& GetStateName() const {
        static std::string empty;
        return stateMachine ? stateMachine->GetCurrentStateName() : empty;
    }
};

// ==============================================================================
// AnimatorSystem — ISystem that updates all AnimatorComponents
// and writes the current frame into SpriteComponent (for the renderer)
// ==============================================================================
class AnimatorSystem : public ISystem {
public:
    const char* GetName() const override { return "AnimatorSystem"; }

    void OnUpdate(World& world, double dt) override {
        world.Each<AnimatorComponent, SpriteComponent>(
            [&](EntityID /*e*/, AnimatorComponent& anim, SpriteComponent& sprite) {
                if (!anim.stateMachine) return;

                anim.stateMachine->Update((float)dt);

                // Push current frame into SpriteComponent
                const SpriteFrame* frame = anim.stateMachine->GetCurrentFrame();
                if (!frame) return;

                // Write UV rect and texture to sprite
                sprite.uvRect  = frame->uvRect;
                sprite.flipX   = anim.stateMachine->GetCurrentFlipX();
                sprite.flipY   = anim.stateMachine->GetCurrentFlipY();
                sprite.pivot   = frame->pivot;

                Texture2D* tex = anim.stateMachine->GetCurrentTexture();
                if (tex) sprite.texture = tex->shared_from_this();
            });
    }

    // Build helpers — create and attach an AnimatorComponent to an entity
    static AnimatorComponent& Attach(World& world, EntityID entity);
    static AnimStateMachine*  GetMachine(World& world, EntityID entity);
};

// ==============================================================================
// AnimatorBuilder — Fluent builder for setting up a state machine
// ==============================================================================
class AnimatorBuilder {
public:
    explicit AnimatorBuilder(AnimStateMachine& sm) : m_sm(sm) {}

    // Add a state with a clip from the library
    AnimatorBuilder& State(const std::string& name, const std::string& clipName);
    AnimatorBuilder& State(const std::string& name, std::shared_ptr<SpriteAnimClip> clip);

    // Add transitions
    AnimatorBuilder& Transition(const std::string& from, const std::string& to,
                                 float duration = AnimConst::DEFAULT_TRANSITION_TIME);
    AnimatorBuilder& OnParam(const std::string& paramName, ConditionOp op,
                              float threshold = 0.0f);  // Chained to previous transition
    AnimatorBuilder& OnBool (const std::string& paramName, bool value);
    AnimatorBuilder& OnTrigger(const std::string& paramName);
    AnimatorBuilder& ExitTime(float normalizedTime);

    AnimatorBuilder& DefaultState(const std::string& name);
    AnimatorBuilder& AnyState   (const std::string& toState,
                                  float duration = AnimConst::DEFAULT_TRANSITION_TIME);

    AnimStateMachine& Build() { return m_sm; }

private:
    AnimStateMachine& m_sm;
    AnimTransition*   m_lastTransition = nullptr;
};

} // namespace BADGE2D
