//  /$$$$$$$   /$$$$$$   /$$$$$$         /$$$$$$   /$$$$$$   /$$$$$$  /$$$$$$$ 
// | $$__  $$ /$$__  $$ /$$__  $$       /$$__  $$ /$$$_  $$ /$$__  $$| $$____/ 
// | $$  \ $$| $$  \__/| $$  \ $$      |__/  \ $$| $$$$\ $$|__/  \ $$| $$      
// | $$$$$$$/| $$      | $$  | $$        /$$$$$$/| $$ $$ $$  /$$$$$$/| $$$$$$$ 
// | $$____/ | $$      | $$  | $$       /$$____/ | $$\ $$$$ /$$____/ |_____  $$
// | $$      | $$    $$| $$  | $$      | $$      | $$ \ $$$| $$       /$$  \ $$
// | $$      |  $$$$$$/|  $$$$$$/      | $$$$$$$$|  $$$$$$/| $$$$$$$$|  $$$$$$/
// |__/       \______/  \______/       |________/ \______/ |________/ \______/ 


#ifndef SHAREDSECTION_H
#define SHAREDSECTION_H

#include <atomic>
#include <QDebug>
#include <gtest/internal/gtest-port.h>

#include <pcosynchro/pcosemaphore.h>

#ifdef USE_FAKE_LOCO
#  include "fake_locomotive.h"
#else
#  include "locomotive.h"
#endif

#ifndef USE_FAKE_LOCO
  #include "ctrain_handler.h"
#endif

#include "sharedsectioninterface.h"

/**
 * @brief La classe SharedSection implémente l'interface SharedSectionInterface qui
 * propose les méthodes liées à la section partagée.
 */
class SharedSection final : public SharedSectionInterface
{
public:
    enum class ExpectedFunction { ACCESS, LEAVE, RELEASE, ANY };
    enum class State { FREE, TAKEN, WAITING_D1, WAITING_D2, TAKEN_BOTH};
    /**
     * @brief SharedSection Constructeur de la classe qui représente la section partagée.
     * Initialisez vos éventuels attributs ici, sémaphores etc.
     */
    SharedSection() : semaphore{1}, mutex{1}, state{State::FREE} {
    }

    /**
     * @brief Request access to the shared section
     * @param Locomotive who asked access
     * @param Direction of the locomotive
     */
    void access(Locomotive& loco, Direction d) override {
        if (&loco == currentLoco || &loco == waitingLoco) {
            errors++;
            return;
        }
        loco.arreter();
        bool willBlock = true;
        mutex.acquire();
        if (state == State::FREE) {
            state = State::TAKEN;
            currentLoco = &loco;
            currentDirection = d;
            nextFunction = ExpectedFunction::LEAVE;
        } else if (state == State::TAKEN) {
            waitingLoco = &loco;
            waitingDirection = d;
            if (d != currentDirection) {
                state = State::TAKEN_BOTH;
                willBlock = false;
                nextFunction = ExpectedFunction::ANY;
            }else {
                state = (d == Direction::D1) ? State::WAITING_D1 : State::WAITING_D2;
                nextFunction = ExpectedFunction::LEAVE;
            }

        }
        mutex.release();
        if (willBlock) {
            semaphore.acquire();
        }
        loco.demarrer();
    }

    /**
     * @brief Notify the shared section that a Locomotive has left (not freed yed).
     * @param Locomotive who left
     * @param Direction of the locomotive
     */
    void leave(Locomotive& loco, Direction d) override {
        mutex.acquire();
        if (nextFunction != ExpectedFunction::LEAVE && nextFunction != ExpectedFunction::ANY) {
            errors++;
        }
        nextFunction = ExpectedFunction::RELEASE;
        if ((d == Direction::D1 && state == State::WAITING_D2) ||
            (d == Direction::D2 && state == State::WAITING_D1)) {
            if ( waitingLoco != &loco && currentLoco != &loco ){ errors++;}
            else if (waitingLoco == &loco && waitingDirection != d) {errors++;}
            else if (currentLoco == &loco && currentDirection != d) {errors++;}

            semaphore.release();
            state = State::TAKEN_BOTH;
            nextFunction = ExpectedFunction::ANY;
        } else {
            if (state == State::TAKEN_BOTH) {
                if ( waitingLoco != &loco && currentLoco != &loco ) { errors++; }
            } else {
                if ( currentLoco != &loco ){ errors++;}
                else if (currentLoco == &loco && currentDirection != d) {errors++;}
            }
        }
        mutex.release();
    }

    /**
     * @brief Notify the shared section that it can now be accessed again (freed).
     * @param Locomotive who sent the notification
     */
    void release(Locomotive &loco) override {
        mutex.acquire();

        if ( waitingLoco != &loco && currentLoco != &loco ){ errors++; mutex.release(); return;}
        if (nextFunction != ExpectedFunction::RELEASE && nextFunction != ExpectedFunction::ANY) {
            errors++;
        }

        switch (state) {
            case State::WAITING_D1:
            case State::WAITING_D2:
                semaphore.release();
            case State::TAKEN_BOTH:
                state = State::TAKEN;
                currentLoco = waitingLoco;
                waitingLoco = nullptr;
                currentDirection = waitingDirection;
                nextFunction = ExpectedFunction::LEAVE;
                break;
            case State::TAKEN:
                state = State::FREE;
                nextFunction = ExpectedFunction::ACCESS;
                currentLoco = nullptr;
                semaphore.release();
                break;
            default:
                break;
        }
        mutex.release();
    }

    /**
     * @brief Stop all locomotives to access this shared section
     */
    void stopAll() override {
        mutex.acquire();
        if (state == State::FREE) {
            semaphore.acquire();
        }

        stopped = true;
        mutex.release();
    }

    /**
     * @brief Return nbErrors
     * @return nbErrors
     */
    int nbErrors() override {
        return errors;
    }
    //getters for easier testing
    Locomotive* getCurrentLoco() const {
        return currentLoco;
    }
    Locomotive* getWaitingLoco() const {
        return waitingLoco;
    }
    Direction getCurrentDirection() const {
        return currentDirection;
    }
    Direction getWaitingDirection() const {
        return waitingDirection;
    }
    State getState() const {
        return state;
    }

private:
    /*
     * Vous êtes libres d'ajouter des méthodes ou attributs
     * pour implémenter la section partagée.
     */
    int errors = 0;
    Locomotive* currentLoco = nullptr;
    Locomotive* waitingLoco = nullptr;
    Direction currentDirection;
    Direction waitingDirection;
    PcoSemaphore semaphore;
    PcoSemaphore mutex;
    bool stopped = false;
    ExpectedFunction nextFunction = ExpectedFunction::ACCESS;
    State state = State::FREE;
};


#endif // SHAREDSECTION_H
