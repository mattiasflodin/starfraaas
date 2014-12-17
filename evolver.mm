<map version="0.7.1">
<node TEXT="Evolver">
<node TEXT="Use cases" POSITION="left">
<node TEXT="Particle system" FOLDED="true">
<node TEXT="Does strategy track particle state?"/>
</node>
<node TEXT="Checkpoint" FOLDED="true">
<node TEXT="Good question..."/>
</node>
<node TEXT="Ordinary object with multiple materials" FOLDED="true">
<node TEXT="Composed of multiple chunks of geometry / materials"/>
</node>
<node TEXT="Subgraph that gets rendered&#xa;to texture (Billboard?)"/>
</node>
<node TEXT="Assumptions" POSITION="right">
<node TEXT="A particular rendering strategy can&#xa;be very tied to hardware" STYLE="fork">
<edge WIDTH="thin"/>
<font NAME="SansSerif" SIZE="12"/>
</node>
</node>
<node TEXT="Goals" POSITION="right">
<node TEXT="Shader / texture sorting&#xa;ought to penetrate objects"/>
<node TEXT="Some way of controlling all object&#xa;parameters via animator(?)"/>
</node>
<node TEXT="Concepts" POSITION="right">
<node TEXT="resource">
<node TEXT="Things allocated on the device"/>
<node TEXT="Not &quot;parameters&quot;"/>
<node TEXT="When do they get allocated / deallocated?">
<node TEXT="When object is required / present"/>
</node>
</node>
<node TEXT="effect">
<node TEXT="Collection of strategies"/>
<node TEXT="Not a type in itself, just a name"/>
<node TEXT="Standardizes the set of resources required"/>
</node>
<node TEXT="strategy"/>
<node TEXT="state_set">
<node TEXT="Exist within device"/>
<node TEXT="Must be created to set states in device"/>
<node TEXT="Provides operator&lt; to sort in the best way for device"/>
<node TEXT="Once created can make state switching more efficient"/>
<node TEXT="Where is state_set stored between strategy::render calls?">
<node TEXT="strategy"/>
<node TEXT="resource_set"/>
<node TEXT="stored in primitive?"/>
<node TEXT="alternatively stored in strategy instance"/>
</node>
</node>
<node TEXT="primitive">
<node TEXT="strategy"/>
<node TEXT="resources"/>
<node TEXT="state set"/>
</node>
<node TEXT="primitive set">
<node TEXT="The closest we get to a scene graph"/>
<node TEXT="Maintains everything that is to be rendered"/>
<node TEXT="Defers rendering to enable sorting"/>
<node TEXT="Does strategy handle primitive insertion?">
<edge WIDTH="thin"/>
<font NAME="SansSerif" SIZE="12"/>
</node>
<node TEXT="How do we manage primitive set membership?">
<node TEXT="&quot;stale&quot; flag"/>
</node>
</node>
<node TEXT="Vertex program"/>
</node>
<node TEXT="Problems" POSITION="left">
<node TEXT="Multi-device rendering">
<node TEXT="Doesn&apos;t occur in practice..."/>
<node TEXT="Should be possible with current design"/>
</node>
<node TEXT="How are non-resource parameters communicated to strategy?"/>
<node TEXT="How to disconnect vertex program&#xa;management from device?">
<node TEXT="Separate facility loads source code">
<node TEXT="Possibly user-provided"/>
</node>
<node TEXT="Make each syntax / version a feature?">
<node TEXT="Alternative:&#xa;shader provides all variants">
<node TEXT="Probably not practical&#xa;- too many versions"/>
</node>
<node TEXT="Shader provides some variants">
<node TEXT="Strategy requests appropriate variant"/>
</node>
</node>
<node TEXT="Device may need to know how what state the VP holds&#xa;(for optimization purposes)"/>
</node>
<node TEXT="One interface per vertex program?">
<node TEXT="would make it easier to set arguments"/>
<node TEXT="Must wrap state_set calls">
<node TEXT="Unless the handling is within the device"/>
</node>
</node>
</node>
<node TEXT="Scene graph?" POSITION="left">
<node TEXT="Out of scope"/>
<node TEXT="We only need primitive set"/>
<node TEXT="What of folded billboards?"/>
<node TEXT="Occlusion culling?"/>
<node TEXT="Where does octree fit in?"/>
</node>
</node>
</map>
