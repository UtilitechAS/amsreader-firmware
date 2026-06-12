<script>
    let europe = ["Amsterdam","Athens","Belfast","Berlin","Bratislava","Brussels","Bucharest","Budapest","Copenhagen","Dublin",
    "Helsinki","Lisbon","Ljubljana","London","Luxembourg","Madrid","Malta","Nicosia","Oslo","Paris","Prague","Riga","Rome",
    "Sofia","Stockholm","Tallinn","Vienna","Vilnius","Warsaw","Zagreb","Zurich"];
</script>

<option>GMT</option>
{#each europe as c}
    <option>Europe/{c}</option>
{/each}
