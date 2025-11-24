# pco25_lab04
## Section critique 
Comme demandé dans l'énoncé, nous avons implementé les fonctions access, leave, release en prennant en compte le fait que celle si sera utilisée par 2 trains seulement.
### Fonction access
```c++
        if (&loco == currentLoco || &loco == waitingLoco) {
            errors++;
            return;
        }
```
ce bloc permet d'identifier si le train qui demande l'accès est déja dans la section critique (ou si c'est celui en attente) et incremente le compteur d'erreurs si c'est le cas.
```c++
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
            loco.arreter();
            semaphore.acquire();
            loco.demarrer();
        }
```
willBlock est un flag qui va indiquer si le train doit s'arreter ou pas. Si la section critique est libre, le train peut y entrer directement. Si elle est prise, on regarde la direction du train en cours et celle du train qui demande l'accès. Si elles sont différentes, on passe à l'état TAKEN_BOTH et le train peut entrer sans s'arreter.
Après ceci, nous avons differents cas:
- Si la section est libre, le train entre directement sans s'arreter.
- si la direction est prise et le train va dans la meme direction, on passe à l'état WAITING_D1 ou WAITING_D2 et le train s'arrete.                  