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
    enum class State { FREE, TAKEN, WAITING_SAME_D, WAITING_DIFFERENT_D};
    /**
     * @brief SharedSection Constructeur de la classe qui représente la section partagée.
     * Initialisez vos éventuels attributs ici, sémaphores etc.
     */
    SharedSection() : semaphore{0}, mutex{1}, state{State::FREE} {
    }

    /**
     * @brief Request access to the shared section
     * @param Locomotive who asked access
     * @param Direction of the locomotive
     */
    void access(Locomotive& loco, Direction d) override {
        if (&loco == currentLoco || &loco == waitingLoco) {errors++;return;}
        bool willBlock = true;
        mutex.acquire();
        if (state == State::FREE) {
            state = State::TAKEN;
            currentLoco = &loco;
            currentDirection = d;
            nextFunction = ExpectedFunction::LEAVE;
            willBlock = false;
        } else if (state == State::TAKEN) {
            waitingLoco = &loco;
            waitingDirection = d;
            if (d != currentDirection) {
                state = State::WAITING_DIFFERENT_D;
            }else {
                state = State::WAITING_SAME_D;
                if (nextFunction == ExpectedFunction::RELEASE) {
                    willBlock = false;
                }
            }
            nextFunction = ExpectedFunction::LEAVE;

        }
        mutex.release();
        if (willBlock) {
            loco.arreter();
            semaphore.acquire();
            mutex.acquire();
            state = State::TAKEN;
            currentLoco = &loco;
            currentDirection = d;
            waitingLoco = nullptr;
            nextFunction = ExpectedFunction::LEAVE;
            mutex.release();
            loco.demarrer();
        }
    }

    /**
     * @brief Notify the shared section that a Locomotive has left (not freed yed).
     * @param Locomotive who left
     * @param Direction of the locomotive
     */
    void leave(Locomotive& loco, Direction d) override {
        mutex.acquire();
        if(currentLoco == &loco && currentDirection != d) {errors++;mutex.release();return;}
        if (nextFunction != ExpectedFunction::LEAVE && nextFunction != ExpectedFunction::ANY) {
            errors++;
        }
        nextFunction = ExpectedFunction::RELEASE;
        if (state == State::WAITING_SAME_D) {
            if ( waitingLoco != &loco && currentLoco != &loco ) { errors++; }
            if (currentLoco == &loco && currentDirection == d) {semaphore.release();}
            else if (waitingLoco == &loco && waitingDirection != d) {errors++; mutex.release();return;}
            nextFunction = ExpectedFunction::ANY;
        } else if (state == State::WAITING_DIFFERENT_D) {
            if (&loco == waitingLoco){errors++; mutex.release();return;}
            if ( currentLoco != &loco ) { errors++; mutex.release();return;}
            if (currentDirection != d) {errors++; mutex.release();return;}
            //semaphore will be released when the first loco calls release
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
            case State::WAITING_DIFFERENT_D:
                state = State::TAKEN;
                currentLoco = waitingLoco;
                currentDirection = waitingDirection;
                waitingLoco = nullptr;
                nextFunction = ExpectedFunction::ACCESS;
                semaphore.release();
                break;
            case State::WAITING_SAME_D:
                state = State::TAKEN;
                currentLoco = waitingLoco;
                currentDirection = waitingDirection;
                waitingLoco = nullptr;
                nextFunction = ExpectedFunction::ANY;
                semaphore.release();
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
