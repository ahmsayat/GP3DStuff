#include "AssimpDemo.h"

// Declare our game instance
AssimpDemo game;

#include "postprocess.h"

AssimpDemo::AssimpDemo()
    : _scene(NULL), _wireframe(false)
{
}

void AssimpDemo::initialize()
{
    // Load game scene from file
    _scene = Scene::load("res/box.gpb");    // This file is just for the cameralevel
    //_scene = Scene::create();
    
    Camera* camera = Camera::createPerspective(45, getAspectRatio(), 0.1f, 350.0f);
    
    // ---------------------------------------
    
    Node* _cameraNode = _scene->findNode("cameralevel");
    _cameraNode->setCamera(camera);
    
    _scene->setActiveCamera(camera);
    SAFE_RELEASE(camera);
    
    Node*oldcn = _scene->findNode("cube");
    _scene->removeNode(oldcn);

    // ---------------------------------------
    
    Assimp::Importer importer;
    
    const aiScene*ascene = loadAiScene(importer, "res/models/box.obj"); // import obj
    Node*node = createNodeFromAiNode(ascene,
                                     "cube",    // There are no nodes in obj files, but this also names the GP node to be created
                                     "res/demo.material#crate"  // Material to be used
                                     );
    node->setScale(6.0);
    node->setTranslation(1.75, 0, 0);
    
    ascene = loadAiScene(importer, "res/models/cone.dae");  // import dae (COLLADA)
    node = createNodeFromAiNode(ascene, "cone1");           // no material part here, because we use multiple materials
    Model*mod = dynamic_cast<Model*>(node->getDrawable());
    if (mod) {
        mod->setMaterial("res/demo.material#conebump", 2);  // Separate materials for the three meshparts
        mod->setMaterial("res/demo.material#coneunlit", 1);
        mod->setMaterial("res/demo.material#coneshiny", 0);
    }
    
    ascene = loadAiScene(importer, "res/models/X/dwarf.x");     // import X
    node = createNodeFromAiNode(ascene, "dwarf");
    mod = dynamic_cast<Model*>(node->getDrawable());
    if (mod) {
        mod->setMaterial("res/demo.material#axe", 0);
        mod->setMaterial("res/demo.material#dwarf", 1);
    }
    node->setScale(0.02);
    node->setTranslation(-1.75, -.5, 0);
    
    // Set the aspect ratio for the scene's camera to match the current resolution
    _scene->getActiveCamera()->setAspectRatio(getAspectRatio());
    
    // Here the Assimp Importer goes out of scope, and should automatically clean up by the importer destructor
}

void AssimpDemo::finalize()
{
    SAFE_RELEASE(_scene);
}

void AssimpDemo::update(float elapsedTime)
{
    // Rotate models
    _scene->findNode("cone1")->rotateY(MATH_DEG_TO_RAD((float)elapsedTime / 1000.0f * 36.0f));
    _scene->findNode("cube")->rotateY(MATH_DEG_TO_RAD((float)elapsedTime / 1000.0f * 32.0f));
    _scene->findNode("dwarf")->rotateY(MATH_DEG_TO_RAD((float)elapsedTime / 1000.0f * -18.0f));

}

void AssimpDemo::render(float elapsedTime)
{
    // Clear the color and depth buffers
    clear(CLEAR_COLOR_DEPTH, Vector4::zero(), 1.0f, 0);

    // Visit all the nodes in the scene for drawing
    _scene->visit(this, &AssimpDemo::drawScene);
}

bool AssimpDemo::drawScene(Node* node)
{
    // If the node visited contains a drawable object, draw it
    Drawable* drawable = node->getDrawable(); 
    if (drawable)
        drawable->draw(_wireframe);

    return true;
}

void AssimpDemo::keyEvent(Keyboard::KeyEvent evt, int key)
{
    if (evt == Keyboard::KEY_PRESS)
    {
        switch (key)
        {
        case Keyboard::KEY_ESCAPE:
            exit();
            break;
        }
    }
}

/**
 * Tries to find aiNode named nodeid in the aiScene and convert it to a GP Node (with Model).
 * If there is no such a node, (some file formats do not even have nodes)
 * loads any meshes there are in the aiScene and packs them to one Model, and Node.
 * If materialProps is present, creates that material and binds to the mesh.
 */
Node* AssimpDemo::createNodeFromAiNode(const aiScene* ascene, const char*nodeid, const char* materialProps, bool addToScene) {
    Node*node = NULL;
    
    Mesh*mesh = loadAiMesh(ascene, nodeid);
    if (mesh) {
        node = Node::create(nodeid);
        Model*model = Model::create(mesh);
        node->setDrawable(model);
        if (materialProps) {
            Material*mat = Material::create(materialProps);
            if (mat) {
                if (mesh->getPartCount() > 1) {
                    for (int i = 0; i < mesh->getPartCount(); i++) {
                        model->setMaterial(mat, i);
                    }
                }
                else {
                    model->setMaterial(mat);
                }
            }
            SAFE_RELEASE(mat);
        }
        SAFE_RELEASE(model);
        SAFE_RELEASE(mesh);
    }
    if (addToScene && node) {
        _scene->addNode(node);
        node->release();
    }
    return node;
}

/**
 Loads a 3D object file and creates an Assimp Scene from it
 */
const aiScene* AssimpDemo::loadAiScene(Assimp::Importer &importer, const char*path) {
    const aiScene* ascene = NULL;
    
    // Setup path to the resources folder
    std::string fullPath;
    if (FileSystem::isAbsolutePath(path)) {
        fullPath.assign(path);
    }
    else {
        fullPath.assign( FileSystem::getResourcePath());
        fullPath += path;
    }

    importer.SetPropertyFloat("PP_GSN_MAX_SMOOTHING_ANGLE", 60); // I think this doesn't have any effect when the meshes already contain normals

    // See Assimp documentation
    // aiProcess_CalcTangentSpace flag makes possible to use normal mapping effects
    
    ascene = importer.ReadFile(fullPath, aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_CalcTangentSpace);
    if (!ascene) {
        GP_WARN("Failed to open %s", fullPath.c_str());
    }
    return ascene;
}

/**
 * Tries to load an aiNode from an aiScene
 * If there is no such a node, loads any meshes there are in the aiScene.
 * returns a GP Mesh
 */
Mesh* AssimpDemo::loadAiMesh(const aiScene* ascene, const char*nodeid) {
    int meshCount = 1;
    aiNode*aino = ascene->mRootNode->FindNode(nodeid);
    if (aino && aino->mNumMeshes > 0) {
        meshCount = aino->mNumMeshes;
    }
    else {
        meshCount = ascene->mNumMeshes;
        GP_WARN("Node %s not found, num of meshes in the scene %d", nodeid, meshCount);
    }
    int meshInd[meshCount];
    if (aino) {
        for (int i = 0; i < aino->mNumMeshes; i++) {
            meshInd[i] = aino->mMeshes[i];
        }
    }
    else {
        for (int i = 0; i < meshCount; i++) {
            meshInd[i] = i;
        }
    }
    
    int vertexCount = 0;
    //const int vertSize = 3+3+2 +6;
    const aiMesh* amesh;
    for (int m = 0; m < meshCount; m++) {
        amesh = ascene->mMeshes[meshInd[m]];
        GP_ASSERT(amesh);
        if ( amesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE) {
            GP_WARN("Only triangles supported");
            continue;
        }
        
        if (!amesh->HasFaces() || !amesh->HasNormals() || !amesh->HasPositions()) {
            GP_WARN("Something missing in the mesh");
            continue;
        }
        GP_WARN("Part %d, NumVertices %d, NumFaces %d", m, amesh->mNumVertices, amesh->mNumFaces);
        vertexCount += amesh->mNumVertices;
        
        if (ascene->mNumMaterials > 0) {
            aiMaterial*aim = ascene->mMaterials[amesh->mMaterialIndex];
            if (aim->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
                aiString tpath;
                aim->GetTexture(aiTextureType_DIFFUSE, 0, &tpath);
                GP_WARN("Texture: %s", tpath.data);
            }
        }
    }
    
    Mesh* mesh = NULL;
    
    if (vertexCount == 0) {
        GP_WARN("No vertices");
    }
    else {
        VertexFormat::Element elements[] =
        {
            VertexFormat::Element(VertexFormat::POSITION, 3),
            VertexFormat::Element(VertexFormat::NORMAL, 3),
            VertexFormat::Element(VertexFormat::TEXCOORD0, 2),
            VertexFormat::Element(VertexFormat::TANGENT,3),
            VertexFormat::Element(VertexFormat::BINORMAL,3),
        };
        int vertSize = 0;
        for (int i = 0; i < sizeof(elements) / sizeof(VertexFormat::Element); i++) {
            vertSize += elements[i].size;
        }
        float vertices[vertexCount * vertSize];
        int vertInd[meshCount];
        
        int vertInd1 = 0;
        for (int m = 0; m < meshCount; m++) {
            amesh = ascene->mMeshes[meshInd[m]];
            vertInd[m] = vertInd1;
            if (collectVertices(amesh, vertices, vertInd1 * vertSize, aiProcess_CalcTangentSpace)) {
                vertInd1 += amesh->mNumVertices;
            }
        }
        
        mesh = Mesh::createMesh(VertexFormat(elements, 5), vertexCount, false);
        if (mesh == NULL) {
            GP_ERROR("Failed to create mesh.");
            return NULL;
        }
        mesh->setVertexData(vertices, 0, vertexCount);
        
        for (int m = 0; m < meshCount; m++) {
            amesh = ascene->mMeshes[meshInd[m]];
            uint32_t index[amesh->mNumFaces * 3];
            uint32_t* ip = index;
            for (int i = 0; i < amesh->mNumFaces; i++) {
                *ip++ = amesh->mFaces[i].mIndices[0] + vertInd[m];
                *ip++ = amesh->mFaces[i].mIndices[1] + vertInd[m];
                *ip++ = amesh->mFaces[i].mIndices[2] + vertInd[m];
            }
            MeshPart* part = mesh->addPart(Mesh::TRIANGLES, Mesh::INDEX32, amesh->mNumFaces * 3, false);
            if (part == NULL) {
                GP_WARN("Failed to create mesh part (with index %d) for mesh .", m);
            }
            else {
                part->setIndexData(index, 0, amesh->mNumFaces * 3);
            }
        }
    }
    return mesh;
}

bool AssimpDemo::collectVertices(const aiMesh*aimesh, float*vert, int startInd, int flags) {
    float *vp = &vert[startInd];
    for (int v = 0; v < aimesh->mNumVertices; v++) {
        *vp++ = aimesh->mVertices[v].x;
        *vp++ = aimesh->mVertices[v].y;
        *vp++ = aimesh->mVertices[v].z;
        *vp++ = aimesh->mNormals[v].x;
        *vp++ = aimesh->mNormals[v].y;
        *vp++ = aimesh->mNormals[v].z;
        aiVector3D vTextureUV;
        if (aimesh->HasTextureCoords(0))
            vTextureUV = aimesh->mTextureCoords[0][v];
        else
            vTextureUV = aimesh->mVertices[v];
        *vp++ = vTextureUV.x;
        *vp++ = vTextureUV.y;
        if (flags & aiProcess_CalcTangentSpace && aimesh->HasTangentsAndBitangents()) {
            *vp++ = aimesh->mTangents[v].x;
            *vp++ = aimesh->mTangents[v].y;
            *vp++ = aimesh->mTangents[v].z;
            *vp++ = aimesh->mBitangents[v].x;
            *vp++ = aimesh->mBitangents[v].y;
            *vp++ = aimesh->mBitangents[v].z;
        }
    }
    return true;
}

void AssimpDemo::touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex)
{
    switch (evt)
    {
    case Touch::TOUCH_PRESS:
        _wireframe = !_wireframe;
        break;
    case Touch::TOUCH_RELEASE:
        break;
    case Touch::TOUCH_MOVE:
        break;
    };
}
