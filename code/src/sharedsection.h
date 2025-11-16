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

    /**
     * @brief SharedSection Constructeur de la classe qui représente la section partagée.
     * Initialisez vos éventuels attributs ici, sémaphores etc.
     */
    SharedSection() : semaphore{1}, mutex{1}{
        // TODO
    }

    /**
     * @brief Request access to the shared section
     * @param Locomotive who asked access
     * @param Direction of the locomotive
     */
    void access(Locomotive& loco, Direction d) override {
        // TODO
        if (&loco == currentLoco || &loco == waitingLoco) {
            errors++;
            return;
        }
        loco.arreter();
        mutex.acquire();
        if (state == State::FREE) {
            state = State::TAKEN;
            currentLoco = &loco;
            currentDirection = d;
        } else if (state == State::TAKEN) {
            state = (d == Direction::D1) ? State::WAITING_D1 : State::WAITING_D2;
            waitingLoco = &loco;
            waitingDirection = d;
        }
        nextFunction = ExpectedFunction::LEAVE;
        mutex.release();
        semaphore.acquire();
        loco.demarrer();
    }

    /**
     * @brief Notify the shared section that a Locomotive has left (not freed yed).
     * @param Locomotive who left
     * @param Direction of the locomotive
     */
    void leave(Locomotive& loco, Direction d) override {
        // TODO
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
            if ( currentLoco != &loco ){ errors++;}
            else if (currentLoco == &loco && currentDirection != d) {errors++;}
        }
        mutex.release();
    }

    /**
     * @brief Notify the shared section that it can now be accessed again (freed).
     * @param Locomotive who sent the notification
     */
    void release(Locomotive &loco) override {
        // TODO
        mutex.acquire();

        if ( waitingLoco != &loco && currentLoco != &loco ){ errors++; return;}
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
        // TODO
        mutex.acquire();
        if (state == State::FREE)
            semaphore.acquire();
        stopped = true;
        mutex.release();
    }

    /**
     * @brief Return nbErrors
     * @return nbErrors
     */
    int nbErrors() override {
        // TODO
        return errors;
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
    enum class ExpectedFunction { ACCESS, LEAVE, RELEASE, ANY };
    ExpectedFunction nextFunction = ExpectedFunction::ACCESS;
    enum class State { FREE, TAKEN, WAITING_D1, WAITING_D2, TAKEN_BOTH};
    State state = State::FREE;
};


#endif // SHAREDSECTION_H
