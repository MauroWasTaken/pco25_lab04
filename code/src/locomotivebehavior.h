//  /$$$$$$$   /$$$$$$   /$$$$$$         /$$$$$$   /$$$$$$   /$$$$$$  /$$$$$$$ 
// | $$__  $$ /$$__  $$ /$$__  $$       /$$__  $$ /$$$_  $$ /$$__  $$| $$____/ 
// | $$  \ $$| $$  \__/| $$  \ $$      |__/  \ $$| $$$$\ $$|__/  \ $$| $$      
// | $$$$$$$/| $$      | $$  | $$        /$$$$$$/| $$ $$ $$  /$$$$$$/| $$$$$$$ 
// | $$____/ | $$      | $$  | $$       /$$____/ | $$\ $$$$ /$$____/ |_____  $$
// | $$      | $$    $$| $$  | $$      | $$      | $$ \ $$$| $$       /$$  \ $$
// | $$      |  $$$$$$/|  $$$$$$/      | $$$$$$$$|  $$$$$$/| $$$$$$$$|  $$$$$$/
// |__/       \______/  \______/       |________/ \______/ |________/ \______/ 

#ifndef LOCOMOTIVEBEHAVIOR_H
#define LOCOMOTIVEBEHAVIOR_H

#include "locomotive.h"
#include "launchable.h"
#include "sharedsectioninterface.h"

/**
 * @brief La classe LocomotiveBehavior représente le comportement d'une locomotive
 */
class LocomotiveBehavior : public Launchable {
public:
    /*!
     * \brief locomotiveBehavior Constructeur de la classe
     * \param loco la locomotive dont on représente le comportement
     */
    LocomotiveBehavior(Locomotive &loco, std::shared_ptr<SharedSectionInterface> sharedSection,
                       Locomotive &locoA) : loco(loco),
                                            sharedSection(sharedSection),
                                            locoA(locoA) {
        // Eventuel code supplémentaire du constructeur
    }

protected:
    inline static const std::array<int, 10> ROUTEBLUE = {1, 31, 30, 29, 28, 22, 21, 20, 19, 13};
    inline static const std::array<int, 12> ROUTERED = {5, 34, 33, 28, 22, 24, 23, 16, 15, 14, 7, 6};
    inline static const std::array<int, 2> SECTIONCRITIQUEBLUE = {30, 20};
    inline static const std::array<int, 2> SECTIONCRITIQUERED = {34, 23};
    static const int LEAVESECTIOND1 = 22;
    static const int LEAVESECTIOND2 = 28;
    inline static const std::array<int, 2> SWITCHESBLUE = {1, 13};
    static const int AGUILLAGETRIGGER = 22;
    static const int AGUILLAGE = 16;

    /*!Static data member ROUTEBLUE cannot have an in-class initializer
     * \brief run Fonction lancée par le thread, représente le comportement de la locomotive
     */
    void run() override;

    /*!
     * \brief printStartMessage Message affiché lors du démarrage du thread
     */
    void printStartMessage() override;

    /*!
     * \brief printCompletionMessage Message affiché lorsque le thread a terminé
     */
    void printCompletionMessage() override;

    /**
     * @brief loco La locomotive dont on représente le comportement
     */
    Locomotive &loco;

    /**
     * @brief sharedSection Pointeur sur la section partagée
     */
    std::shared_ptr<SharedSectionInterface> sharedSection;

    /*
     * Vous êtes libres d'ajouter des méthodes ou attributs
     *
     * Par exemple la priorité ou le parcours
     */

    Locomotive &locoA;
};

#endif // LOCOMOTIVEBEHAVIOR_H
