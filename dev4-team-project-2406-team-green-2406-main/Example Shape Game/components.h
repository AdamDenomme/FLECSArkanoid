// components.h defines the data used by the game

// defines the main elements needed for a gameloop
struct GameLoop {
	GW::SYSTEM::GWindow window;
};

// main graphics engine driving the game
struct GraphicsEngine {
	float clear_depth = 1;
	float clear_color[4] = { 0.15f, 0.15f, 0.15f, 1 };
	GW::GRAPHICS::GDirectX11Surface surface;
};

// The basic elements of our game we interact with and draw
// Using inheritance over containment here just to keep the code cleaner
// Were this not an example I would probably just use the base types directly
struct LINE : public GW::MATH2D::GLINE2F {
	template<typename... any> // this allows me to forward initializer lists
	LINE(any&&... all) : GW::MATH2D::GLINE2F(std::forward<any>(all)...) {}
};
struct CIRCLE : public GW::MATH2D::GCIRCLE2F {
	template<typename... any> // this allows me to forward initializer lists
	CIRCLE(any&&... all) : GW::MATH2D::GCIRCLE2F(std::forward<any>(all)...) {}
};
struct RECTANGLE : public GW::MATH2D::GRECTANGLE2F {
	template<typename... any> // this allows me to forward initializer lists
	RECTANGLE(any&&... all) : GW::MATH2D::GRECTANGLE2F(std::forward<any>(all)...) {}
};
struct COLOR : public GW::MATH::GVECTORF {
	COLOR() { x = 1;  y = 1; z = 1; w = 1; } // default white
	template<typename... any> // this allows me to forward initializer lists
	COLOR(any&&... all) : GW::MATH::GVECTORF(std::forward<any>(all)...) {}
};
// How many other shapes are touching you?
struct HIT_COUNT {
	unsigned int hits = 0;
};
// tag for sorting entities by use case
struct GAME_OBJECT {};
struct HIGHLIGHTED {};
struct SELECTED {};
// main input/interation tool
struct CURSOR {
	enum class MODE { HIGHLIGHT, DRAG } mode;
	GW::INPUT::GBufferedInput mouse;
	GW::CORE::GEventCache events;
};

// Renderer used for debugging
struct DebugRenderer {
	GW::MATH::GMATRIXF camera;
	GW::MATH::GMATRIXF projection;
	GW::MATH::GMATRIXF viewProjection;
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> vertexFormat;
	Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer;
	struct debug_vertex { // internal debug vertex
		GW::MATH2D::GVECTOR2F pos;
		GW::MATH::GVECTORF clr;
	}; // caches world space shapes converted to lines
	std::vector<debug_vertex> cpuCache;
	unsigned gpuVertexCapacity = 0; // what is currently possible
};