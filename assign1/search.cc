#include <vector>
#include <iostream>
#include <string>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include "path.h"
#include "string.h"
#include "imdb.h"
using namespace std;

static const int kWrongArgumentCount = 1;
static const int kSourceTargetSame = 2;
static const int kDatabaseNotFound = 3;
static const int MAX_DEGREE = 6;


static path backTrace(const unordered_map<string, pair<string, film>>& parents, const string& startPlayer, const string& endPlayer) {
  path p(endPlayer);
  while (p.getLastPlayer() != startPlayer) {
    string lastPlayer = p.getLastPlayer();
    pair<string, film> pa = parents.at(lastPlayer);
    p.addConnection(pa.second, pa.first);
  }
  p.reverse();
  return p;
}

static path findPath(const imdb& db, const string& startPlayer, const string& endPlayer) {
  struct filmHash {
    size_t operator()(film f) const {
      return hash<string>()(f.title) ^ hash<int>()(f.year);
    }
  };
  unordered_set<string> visitedActors;
  unordered_set<film, filmHash> visitedMovies;
  unordered_map<string, pair<string, film>> parents;
  queue<pair<string,int>> q;
  q.push(pair<string, int>(startPlayer, 0));
  visitedActors.insert(startPlayer);
  while (!q.empty()) {
    pair<string, int> pa = q.front();
    q.pop();
    string player = pa.first;
    int degree = pa.second;
    vector<film> credits;
    db.getCredits(player, credits);
    for (film f: credits) {
      if (visitedMovies.find(f) == visitedMovies.end()) {
        visitedMovies.insert(f);
        vector<string> cast;
        db.getCast(f, cast);
        for (string actor: cast) {
          if (visitedActors.find(actor) == visitedActors.end()) {
            parents[actor] = pair<string, film>(player, f);
            if (actor == endPlayer) return backTrace(parents, startPlayer, endPlayer);
            if (degree + 1 < MAX_DEGREE) {
              q.push(pair<string, int>(actor, degree + 1));
            }
            visitedActors.insert(actor);
          }
        }
      }
    }
  }
  return path(startPlayer);
}

static bool sanity(const imdb& db, const string& startPlayer, const string& endPlayer) {
  vector<film> credits;
  if (!db.getCredits(startPlayer, credits) || credits.size() == 0) return false;
  credits.clear();
  if (!db.getCredits(endPlayer, credits) || credits.size() == 0) return false;
  return true;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    cerr << "Usage: " << argv[0] << " <source-actor> <target-actor>" << endl;
    return kWrongArgumentCount;
  }

  if (strcmp(argv[1], argv[2]) == 0) {
    cerr << "Ensure that source and target actors are different!" << endl;
    return kSourceTargetSame;
  }

  imdb db(kIMDBDataDirectory);
  if (!db.good()) {
    cerr << "Failed to properly initialize the imdb database." << endl;
    cerr << "Please check to make sure the source files exist and that you have permission to read them." << endl;
    return kDatabaseNotFound;
  }

  string startPlayer = argv[1];
  string endPlayer = argv[2];

  string msgNoPathFound = "No path between those two people could be found.";
  if (!sanity(db, startPlayer, endPlayer)) {
    cout << msgNoPathFound << endl;
  } else {
    path p = findPath(db, startPlayer, endPlayer);
    if (p.getLength() == 0) {
      cout << msgNoPathFound << endl;
    } else {
      cout << p;
    }
  }

  return 0;
}
