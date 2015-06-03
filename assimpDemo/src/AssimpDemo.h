#ifndef AssimpDemo_H_
#define AssimpDemo_H_

// assimp
#include "Importer.hpp"
#include "scene.h"
#include "mesh.h" // assimp mesh

#include "gameplay.h"

//class aiMesh;

using namespace gameplay;

/**
 * Main game class.
 */
class AssimpDemo: public Game
{
public:

    /**
     * Constructor.
     */
    AssimpDemo();

    /**
     * @see Game::keyEvent
     */
	void keyEvent(Keyboard::KeyEvent evt, int key);
	
    /**
     * @see Game::touchEvent
     */
    void touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);

protected:

    /**
     * @see Game::initialize
     */
    void initialize();

    /**
     * @see Game::finalize
     */
    void finalize();

    /**
     * @see Game::update
     */
    void update(float elapsedTime);

    /**
     * @see Game::render
     */
    void render(float elapsedTime);
    
    const aiScene* loadAiScene(Assimp::Importer &importer, const char*path);

    Mesh* loadAiMesh(const aiScene* ascene, const char*nodeid);
    Node* createNodeFromAiNode(const aiScene* ascene, const char*nodeid, const char* materialProps = NULL, bool addToScene = true);
    bool collectVertices(const aiMesh*aimesh, float*vert, int startInd, int flags);


private:

    /**
     * Draws the scene each frame.
     */
    bool drawScene(Node* node);

    Scene* _scene;
    bool _wireframe;
};

#endif
